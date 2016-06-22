#!/bin/sh

cat<<EOF

For this to work, you need checkinstall and help2man along with the
real fac dependencies.  You also need to build fac before attempting
to create a .deb file.

EOF

set -ev

LDFLAGS=-static ./fac fac fac.1

fakeroot checkinstall -D --pkglicense=gplv2+ --pkgname fac -y --strip=yes --deldoc=yes --deldesc=yes --delspec=yes --install=no sh install.sh

