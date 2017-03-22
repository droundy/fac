#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

../../fac --version

../../fac --version > version

grep 'fac version' version

git describe --dirty

grep `git describe --dirty` version

exit 0
