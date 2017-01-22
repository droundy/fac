#!/bin/sh

set -ev

echo $0

# This test ensures that the getting-started tutorial actually works
# right.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cp ../../fac .

if ! which pdflatex; then
    echo there is no latex
    exit 137
fi

if ! test -f /usr/share/dict/words; then
    echo there is no /usr/share/dict/words
    exit 137
fi

# Setting the PATH in the following ensures that we call our
# newly-built fac, rather than one that is already installed.
PATH=`pwd`:$PATH python3 ../getting-started.py ../../web/getting-started.md
