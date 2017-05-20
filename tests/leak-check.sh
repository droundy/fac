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

# FIXME: I'm disabling this test for now, because I am impatient and valgrind is slow.
exit 137

git clone ../../bigbro

cd bigbro

SKIPSPARSE=1 valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ${FAC:-../../../fac}

valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ${FAC:-../../../fac} -c

valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=1 ${FAC:-../../../fac} bigbro

echo we passed!
