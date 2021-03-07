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

import sys
import signal
from PySide6.QtCore import QCoreApplication, QUrl, Slot
from PySide6.QtRemoteObjects import QRemoteObjectHost
from simple import SimpleSource, SimpleSimpleSource


class Simple(SimpleSimpleSource):

    def __init__(self, _i, _f):
        super().__init__()
        self.setI(_i)
        self.setF(_f)

    @Slot()
    def reset(self):
        print(f'Derived Reset() called')


if __name__ == '__main__':
    app = QCoreApplication(sys.argv)
    node = QRemoteObjectHost(QUrl("tcp://127.0.0.1:5005"))
    toosimple = SimpleSource()      # This should fail when abc is working
    simple = SimpleSimpleSource()   # This should fail, too
    working = Simple(3, 3.14)
    print(f'initial: i = {working.i}, f = {working.f}')
    node.enableRemoting(working, "DifferentName")
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    sys.exit(app.exec())
