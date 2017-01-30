#!/bin/sh

set -ev

echo $0

# This test ensures that the fac-with-git-hooks tutorial actually works
# right.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cp ../../fac .

if ! which sparse; then
    echo there is no sparse
    exit 137
fi

# Setting the PATH in the following ensures that we call our
# newly-built fac, rather than one that is already installed.
PATH=`pwd`:$PATH python3 ../getting-started.py ../../web/fac-with-git-hooks.md
