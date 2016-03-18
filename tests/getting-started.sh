#!/bin/sh

set -ev

echo $0

# This test ensures that the getting-started tutorial actually works
# right.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

python3 ../getting-started.py ../../web/getting-started.md
