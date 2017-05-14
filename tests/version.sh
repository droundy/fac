#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

${FAC:-../../fac} --version

${FAC:-../../fac} --version > version

grep 'fac' version

git describe --dirty

grep `git describe --dirty` version

exit 0
