#!/bin/sh

set -ev

echo $0

# This test ensures that there are no memory leaks

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

if ! which valgrind; then
    echo there is no valgrind
    exit 137
fi

git clone ../../bigbro

cd bigbro

echo this is a bug here >> bigbro.h

../../../fac --continual > continual-output &

sleep 20

cat continual-output

grep 'Build failed' continual-output

if grep 'Build succeeded' continual-output; then
    echo should not succeed
    exit 1
fi

git checkout bigbro.h

# Sadly I don't see another solution to this than to guess at how long
# the build will take.  Trouble is we waste time if we guess too long,
# but the test may fail if we guess too short.
sleep 30

cat continual-output

grep 'Build succeeded' continual-output

echo we passed!
