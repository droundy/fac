#!/bin/sh

cat<<EOF

For this to work, you need checkinstall and help2man along with the
real fac dependencies.  You also need to build fac before attempting
to create a .deb file.

EOF

set -ev

checkinstall -D --fstrans=yes --pkglicense=gplv2+ --pkgname fac -y --strip=yes --deldoc=yes --deldesc=yes --delspec=yes --install=no --pakdir=web --pkgversion=`git describe --dirty` sh build/install.sh
ln -sf fac_`git describe --dirty`-1_amd64.deb web/fac-latest.deb

