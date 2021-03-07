#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2021 Ford Motor Company
## Contact: https://www.qt.io/licensing/
##
## This file is part of the QtRemoteObjects module of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:BSD$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## BSD License Usage
## Alternatively, you may use this file under the terms of the BSD license
## as follows:
##
## "Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##   * Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in
##     the documentation and/or other materials provided with the
##     distribution.
##   * Neither the name of The Qt Company Ltd nor the names of its
##     contributors may be used to endorse or promote products derived
##     from this software without specific prior written permission.
##
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
##
## $QT_END_LICENSE$
##
#############################################################################

import argparse
import asyncio
import collections
import logging
import signal
import socket
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import IntEnum
from functools import partial
from struct import pack, unpack, calcsize
from urllib.parse import urlparse
from weakref import WeakSet

DEFAULT_URL = "tcp://127.0.0.1:5005"
logging.basicConfig(format='%(asctime)s.%(msecs)06d %(levelname)-8s %(message)s',
                    datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__)


class PacketType(IntEnum):
    Invalid = 0
    Handshake = 1
    InitPacket = 2
    InitDynamicPacket = 3
    AddObject = 4
    RemoveObject = 5
    InvokePacket = 6
    InvokeReplyPacket = 7
    PropertyChangePacket = 8
    ObjectList = 9
    Ping = 10
    Pong = 11


class Decoder:
    _QVariantDecoders = {
        1: lambda self: self.decodeBool(),
        2: lambda self: self.decodeS32(),
        3: lambda self: self.decodeU32(),
        6: lambda self: self.decodeDouble(),
        10: lambda self: self.decodeQString(),
        12: lambda self: self.decodeByteArray(),
        36: lambda self: self.decodeU16(),
        38: lambda self: self.decodeDouble(), # float is treated as double in QDataStream
    }

    def __init__(self, data):
        self.data = data
        self.index = 0

    def decodeU16(self):
        val = unpack(">H", self.data[self.index:self.index+2])[0]
        logger.debug(f"decodeU16: val = {val} (at index = {self.index})")
        self.index += 2
        return val

    def decodeU32(self):
        val = unpack(">I", self.data[self.index:self.index+4])[0]
        logger.debug(f"decodeU32: val = {val} (at index = {self.index})")
        self.index += 4
        return val

    def decodeS32(self):
        val = unpack(">i", self.data[self.index:self.index+4])[0]
        logger.debug(f"decodeS32: val = {val} (at index = {self.index})")
        self.index += 4
        return val

    def decodeDouble(self):
        val = unpack(">d", self.data[self.index:self.index+8])[0]
        logger.debug(f"decodeDouble: val = {val} (at index = {self.index})")
        self.index += 8
        return val

    def decodeQString(self):
        l = self.decodeU32()
        s = self.data[self.index:self.index+l].decode('utf-16be')
        logger.debug(f"decodeQString: len = {l/2}, s = {s}")
        self.index += l
        return s

    def decodeByteArray(self):
        l = self.decodeU32()
        s = self.data[self.index:self.index+l]
        logger.debug(f"decodeByteArray: len = {l}, s = {s}")
        self.index += l
        return s

    def decodeBool(self):
        b = bool(self.data[self.index])
        logger.debug(f"decodeBool: result {b}")
        self.index += 1
        return b

    def decodeQVariant(self):
        typeId = self.decodeU32()
        isNull = self.decodeBool()
        decoder = Decoder._QVariantDecoders.get(typeId)
        if decoder:
            return decoder(self)
        else:
            raise RuntimeError(f"Tried to decode unrecognized QVariant type ({typeId}).")

    def decodeArgs(self):
        parameters = []
        nArgs = self.decodeU32()
        for i in range(nArgs):
            parameters.append(self.decodeQVariant())
        return parameters


class Timer:
    # Based on https://stackoverflow.com/a/45430833/169296

    def __init__(self, timeout, callback):
        self._timeout = timeout
        self._callback = callback
        self._task = asyncio.create_task(self._job())

    async def _job(self):
        await asyncio.sleep(self._timeout)
        await self._callback()

    async def restart(self):
        if not self._task.cancelled():
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
        self._task = asyncio.create_task(self._job())


class Pipe(ABC):
    handlers = {}
    schema = None

    def __init__(self, url, queue, hbInterval):
        self.url = url
        self.queue = queue
        self.hbInterval = hbInterval
        self.handshake = False
        self.error = self.reader = self.writer = self.timer = None

    @abstractmethod
    async def connect(self):
        pass

    async def run(self):
        await self.connect()
        while not self.error:
            data = await self.reader.readexactly(4)
            if not self.handshake: # Test Qt6 handshake
                size, packetId, rev, firstChar = unpack(">BBBc", data)
                logger.debug(f"Handshake size = {size} id = {packetId} rev = {rev} firstChar = "
                             f"{firstChar}")
                if size < 3 or size > 252 or packetId != 1:
                    size = unpack(">I", data)[0]
                    firstChar = b'\x00'
                else:
                    size -= 3 # size includes packetId, rev and firstChar which we've already read
            else:
                size = unpack(">I", data)[0]
            logger.debug(f"Packet length {size}")
            data = await self.reader.readexactly(size)
            if not self.handshake:
                if firstChar == b'\x00':
                    packetId = unpack(">H", data[:2])[0]
                    if packetId != PacketType.Handshake:
                        logger.error("Invalid pipe, unable to establish handshake")
                        self.error = True
                        continue
                else:
                    # Make it look like the old style handshake packet
                    codec = (firstChar + data).decode('utf-8').encode('utf-16be')
                    data = b'\x00\x01' + pack('>I', len(codec)) + codec
                    logger.error(f"Data {data}")
                self.handshake = True # The Node will delete us if the handshake isn't valid
                if self.hbInterval > 0:
                    self.timer = Timer(self.hbInterval, self.sendHeartbeat)
            elif self.timer:
                await self.timer.restart()
            await self.queue.put( (id(self), data) )
        self.writer.close()
        self.handshake = False
        await self.queue.put( (id(self), None) )

    async def sendHeartbeat(self):
        logger.debug("Sending Heartbeat")
        self.writer.write(pack(">IH", 2, PacketType.Ping))
        await self.timer.restart()

    @classmethod
    def __init_subclass__(cls, **kwargs):
        """
        This creates a map of schema name ("tcp", "ws", etc) to the class (derived from Pipe)
        that handles that type of pipe.

        This classmethod is called whenever a class derived from Pipe is parsed, assuming each
        class has a class variable (not an instance variable) named `schema`.
        """
        super().__init_subclass__(**kwargs)
        if cls.schema is None:
            raise RuntimeError(f"Invalid Pipe defined ({cls.__name__}).  Pipes must declare a "
                               f"schema to support.")
        if cls.schema in Pipe.handlers:
            raise RuntimeError(f"Invalid Pipe defined ({cls.__name__}).  Schema {cls.schema} "
                               f"already in use.")
        Pipe.handlers[cls.schema] = cls


class TcpPipe(Pipe):
    schema = "tcp"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        urlmeta = urlparse(self.url)
        self.hostname = urlmeta.hostname
        self.port = urlmeta.port

    async def connect(self):
        # TODO error handling, reconnect, etc
        self.reader, self.writer = await asyncio.open_connection(self.hostname, self.port)


class Node:

    def __init__(self, name=""):
        self.Handlers = {
            PacketType.Handshake: lambda decoder: self.onHandshake(decoder),
            PacketType.InitPacket: lambda decoder: self.onInitPacket(decoder),
            PacketType.InitDynamicPacket: lambda decoder: self.onInitDynamicPacket(decoder),
            PacketType.RemoveObject: lambda decoder: self.onRemoveObject(decoder),
            PacketType.InvokePacket: lambda decoder: self.onInvokePacket(decoder),
            PacketType.InvokeReplyPacket: lambda decoder: self.onInvokeReplyPacket(decoder),
            PacketType.PropertyChangePacket: lambda decoder: self.onPropertyChangePacket(decoder),
            PacketType.ObjectList: lambda decoder: self.onObjectList(decoder),
            PacketType.Pong: lambda decoder: self.onPong(decoder),
        }
        self.name = name
        # We set write() and currentPipe to the pipe of the packet we are handling
        self.write = self.currentPipe = None
        self.hbInterval = 0
        self.queue = asyncio.Queue()
        self.pipes = {}
        self.replicaPrivateInstances = {}

    def acquire(self, cls, name=None):
        name = name or cls.__name__
        replicaPrivate = self.replicaPrivateInstances.get(name)
        if replicaPrivate is None:
            replicaPrivate = ReplicaPrivate(cls, name)
            self.replicaPrivateInstances[name] = replicaPrivate
        return replicaPrivate.create()

    def setHeartbeatInterval(self, timeout):
        self.hbInterval = timeout

    def connect(self, url):
        urlmeta = urlparse(url)
        if urlmeta.scheme not in Pipe.handlers:
            logger.error(f"Unrecognized schema ({urlmeta.scheme}) for url ({url})")
            logger.error(f"  Known schemas: {Pipe.handlers.keys()}")
            return
        pipe = Pipe.handlers[urlmeta.scheme](url, self.queue, self.hbInterval)
        self.pipes[id(pipe)] = pipe
        asyncio.get_event_loop().create_task(pipe.run())

    def onPipeError(self, msg):
        logger.error(msg)
        del self.pipes[id(self.currentPipe)]

    async def run(self):
        while True:
            pipeId, data = await self.queue.get()
            if data is None:
                del self.pipes[pipeId]
            d = Decoder(data)
            packetId = d.decodeU16()
            handler = self.Handlers.get(packetId, None)
            if handler:
                self.currentPipe = self.pipes.get(pipeId)
                if self.currentPipe is None:
                    continue # We deleted the pipe, but there were still packets in the queue
                self.write = self.currentPipe.writer.write
                logger.debug(f"{PacketType(packetId).name} packet received on node {self.name}")
                handler(d)
                self.write = self.currentPipe = None
                logger.debug("")
            else:
                self.onPipeError(f"Invalid packet found with id = {packetId}")

    def onHandshake(self, d):
        handshake = d.decodeQString()
        if handshake not in ["QtRO 1.3", "QDataStream"]:
            self.onPipeError(f"Unrecognized handshake ({handshake}) received.")

    def onObjectList(self, d):
        l = d.decodeU32()
        i = 0
        while l > 0:
            i += 1
            l -= 1
            name = d.decodeQString()
            typename = d.decodeQString()
            checksum = d.decodeByteArray()
            logger.debug(f"Object #{i}: name: {name} type: {typename} checksum: {checksum}")
        len2 = len(name) * 2
        self.write(pack(f">IHI{len2}sc", 7 + len2, PacketType.AddObject, len2,
                        name.encode('utf-16be'), b'\x00'))

    def onInitPacket(self, d):
        name = d.decodeQString()
        properties = d.decodeArgs()
        logger.info(f"Properties for {name}: {properties}")
        self.replicaPrivateInstances[name].initialize(self.currentPipe, properties)

    def onInitDynamicPacket(self, d): pass
    def onRemoveObject(self, d): pass
    def onInvokePacket(self, d):
        name = d.decodeQString()
        call = d.decodeS32()
        index = d.decodeS32()
        parameters = d.decodeArgs()
        serialId = d.decodeS32()
        propertyIndex = d.decodeS32()
        logger.info(f"Invoke: name: {name} call: {call} index: {index} serialId: {serialId} "
                    f"propertyIndex: {propertyIndex}")
        self.replicaPrivateInstances[name].invoke(index, *parameters)

    def onInvokeReplyPacket(self, d): pass
    def onPropertyChangePacket(self, d):
        name = d.decodeQString()
        index = d.decodeS32()
        val = d.decodeQVariant()
        logger.debug(f"PropertyChange: name: {name} index: {index} value: {val}")

    def onPong(self, d): pass


class Signal:

    def __init__(self):
        self._slots = collections.deque()

    def __repr__(self):
        res = super().__repr__()
        if self._slots:
            extra = f'slots:{len(self._slots)}'
        return f'<{res[1:-1]} [{extra}]>'

    def emit(self, *args):
        for slot in self._slots:
            slot(*args)

    def connect(self, func):
        self._slots.append(func)


class ReplicaPrivate:
    """
    There can be multiple instances of the same replica type.  We don't want
    each to each to talk to the source indepenently, so the ReplicaPrivate class
    represents
      * The pipe (if available) to the source
      * The "cache" of property values locally, shared by instances
      * The link to all instances
    """

    def __init__(self, cls, name):
        self.instances = WeakSet()
        self.pipe = None
        self.cls = cls
        self.name = name
        self.typename = cls.__name__
        self.properties = cls.defaults()

    def create(self):
        replica = self.cls(self)
        self.instances.add(replica)
        return replica

    def write(self, buffer):
        if self.pipe is None:
            return
        self.pipe.writer.write(buffer)

    def initialize(self, pipe, values):
        self.pipe = pipe
        self.properties = values
        for instance in self.instances:
            instance.initialize()

    def invoke(self, index, *parameters):
        signal = self.cls.Signals[index]
        for instance in self.instances:
            print("invoke on instance", instance)
            getattr(instance, signal).emit(*parameters)

    def isValid(self):
        return self.pipe is not None


@dataclass(eq=False)
class Replica:
    _priv: ReplicaPrivate = field(repr=False)
    initialized: Signal = field(init=False, repr=False, default_factory=Signal)

    def __new__(cls, replicaPrivate=None):
        if replicaPrivate is None:
            raise RuntimeError("Replicas can only be created from a Node's acquire() method")
        return super().__new__(cls)

    def name(self):
        return self._priv.name

    def typename(self):
        return self._priv.typename

    def initialize(self, *args):
        self.initialized.emit()

    def callSlot(self, index, *args):
        # 0 is Call.InvokeMetaMethod
        self.sendInvokePacket(index, 0, self._priv.cls.slotTypes()[index], *args)

    def callSetter(self, index, *args):
        # 2 is Call.WriteProperty
        self.sendInvokePacket(index, 2, self._priv.cls.propTypes()[index], *args)

    def sendInvokePacket(self, index, callIndex, types, *args):
        if not self._priv.isValid():
            logger.warning(f"Replica instance (name: {self.name()} type: {self.typename()}) tried "
                           f"to write before pipe available.")
            return
        encodedLen = len(self.name())*2
        prefixPart = [
            ("I", 0),                                           # Length of packet, filled later
            ("H", PacketType.InvokePacket),                     # PacketId
            ("I", encodedLen),                                  # Length of name as a QString
            (f"{encodedLen}s", self.name().encode('utf-16be')), # Replica name string
            ("i", callIndex),                                   # Call
            ("i", index),                                       # Index
            ("I", len(args)),                                   # Number of args
        ]
        argsPart = []
        for ind, arg in enumerate(args):
            f = types[ind]
            if f == "i":
                argsPart.append( ("i", 2) ) # Qt typeId for int
            elif f == "f":
                argsPart.append( ("i", 38) ) # Qt typeId for float
            argsPart.append( ("c", b'\x00') ) # isNull
            if f == "i":
                argsPart.append( ("i", arg) )
            elif f == "f":
                argsPart.append( ("d", arg) )
        suffixPart = [
            ("i", -1), # SerialId
            ("i", -1), # PropertyIndex
        ]
        f, newArgs = map(list, zip(*prefixPart, *argsPart, *suffixPart))
        f = ">" + " ".join(f)
        newArgs[0] = calcsize(f) - 4 # Exclude the 4 bytes for the packet length
        buffer = pack(f, *newArgs)
        self._priv.write(buffer)


@dataclass(eq=False)
class Simple(Replica):
    Signals = ['iChanged', 'fChanged', 'random']
    iChanged: Signal = field(init=False, repr=False, default_factory=Signal)
    fChanged: Signal = field(init=False, repr=False, default_factory=Signal)
    random: Signal = field(init=False, repr=False, default_factory=Signal)

    @property
    def i(self):
        return self._priv.properties[0]

    def pushI(self, i):
        print('pushI', i)
        self.callSlot(0, i)

    @property
    def f(self):
        return self._priv.properties[1]

    @f.setter
    def f(self, f):
        print('f setter', f)
        self.callSetter(1, f)

    def reset(self):
        print('Calling reset slot')
        self.callSlot(1)

    @classmethod
    def defaults(cls):
        return [2, -1.]

    @classmethod
    def propTypes(cls):
        return [
            ('i',), # i is of type int -> i
            ('f',), # f is of type float -> f
        ]

    @classmethod
    def signalTypes(cls):
        return [
            ('i',),
            ('f',),
            ('i',),
        ]

    @classmethod
    def slotTypes(cls):
        return [
            ('i',),
            (),
        ]

    @classmethod
    def signature(cls):
        return b'c6f33edb0554ba4241aad1286a47c8189d65c845'

    @classmethod
    def name(cls):
        return 'Simple'


def handle_exception(loop, context):
    msg = context.get("exception", context["message"])
    logger.error(f"Caught exception: {msg}")
    logger.error(f"{context}")
    logger.info("Shutting down...")
    asyncio.create_task(shutdown(loop))


async def shutdown(loop, signal=None):
    if signal:
        logger.info(f"Received exit signal {signal.name}...")
    logger.info("Cancelling outstanding tasks")
    tasks = [t for t in asyncio.all_tasks() if t is not asyncio.current_task()]
    [task.cancel() for task in tasks]

    await asyncio.gather(*tasks, return_exceptions=True)
    loop.stop()


def handler(i, val):
    print(f"handler #{i} fired {val}")


def main(args):
    loop = asyncio.get_event_loop()
    signals = (signal.SIGHUP, signal.SIGTERM, signal.SIGINT)
    for s in signals:
        loop.add_signal_handler(
            s, lambda s=s: asyncio.create_task(shutdown(loop, signal=s))
        )
    loop.set_exception_handler(handle_exception)
    node  = Node("A")
    if args.heartbeat:
        node.setHeartbeatInterval(args.heartbeat)
    s = node.acquire(Simple, "DifferentName")
    print(f"Initial Simple: i = {s.i} f = {s.f}")
    s.iChanged.connect(partial(handler, 3))
    s.iChanged.connect(partial(handler, 7))
    # s.reset() # This gives a warning about the pipe not being available
    s.initialized.connect(s.reset)
    s.initialized.connect(lambda : print(f"Now s: i = {s.i} f = {s.f}"))
    s2 = node.acquire(Simple, "DifferentName")
    s2.iChanged.connect(partial(handler, 42))
    async def change():
        await asyncio.sleep(1.5)
        s2.f = 99.5
        await asyncio.sleep(.5)
        s2.f = 93.9
    s2.initialized.connect(lambda : loop.create_task(change()))
    node.connect(DEFAULT_URL)
    try:
        loop.create_task(node.run())
        loop.run_forever()
    finally:
        loop.close()
        logger.info("Loop exited")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="provide verbose information about the run (may be repeated)")
    parser.add_argument("--heartbeat", type=int, default=0,
                        help="set heartbeat timeout (in seconds)")
    args = parser.parse_args()
    if args.verbose == 1:
        logger.setLevel(logging.INFO)
    elif args.verbose > 1:
        logger.setLevel(logging.DEBUG)
    main(args)
