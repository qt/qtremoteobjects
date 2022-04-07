#!/bin/sh
#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# Generate the CA key
openssl genrsa -out rootCA.key 2048
# Generate the CA cert
openssl req -x509 -key rootCA.key -out rootCA.pem -sha256 -nodes -subj "/CN=QtRO CA" -days 836

# genFiles stem [extra args to signing]
genFiles () {
    stem=$1
    shift
    # Generate key
    openssl genrsa -out $stem.key 2048
    # Generate certificate-signing request
    openssl req -new -key $stem.key -out $stem.csr -subj "/CN=127.0.0.1"
    # Generate and sign the certificate
    openssl x509 -req -in $stem.csr -out $stem.crt \
                 -CA rootCA.pem -CAkey rootCA.key -CAcreateserial -days 825 -sha256 "$@"
    # Delete the signing request, no longer needed
    rm $stem.csr
}
genFiles server -extfile server-req.ext
genFiles client

dest1="../../../../examples/remoteobjects/ssl/sslserver/cert/"
dest2="../../../../examples/remoteobjects/websockets/common/cert/"

cp -f "client.crt" $dest1
cp -f "client.crt" $dest2
cp -f "client.key" $dest1
cp -f "client.key" $dest2

cp -f "server.crt" $dest1
cp -f "server.crt" $dest2
cp -f "server.key" $dest1
cp -f "server.key" $dest2

cp -f "rootCA.pem" $dest1
cp -f "rootCA.key" $dest1
cp -f "rootCA.srl" $dest1
cp -f "rootCA.pem" $dest2
cp -f "rootCA.key" $dest2
cp -f "rootCA.srl" $dest2
