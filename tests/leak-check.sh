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

SKIPSPARSE=1 valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ../../../fac

valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ../../../fac -c

valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ../../../fac bigbro

echo we passed!
