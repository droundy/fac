#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.fac <<EOF
| echo \$CC > compiler

EOF
cat top.fac

git init
git add top.fac

export CC=gcc

../../fac

grep gcc compiler

CC=clang ../../fac

grep clang compiler

../../fac

grep gcc compiler

exit 0
