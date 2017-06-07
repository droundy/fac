#!/bin/sh

set -ev

if ! which tup; then
    echo there is no tup
    exit 137
fi

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > build.fac <<EOF
| cat input > awesome
> awesome

| wc input > data

| cat data awesome > great && echo silliness > foo
< data
< awesome
EOF

cat > input <<EOF
a
b
c
EOF

git init
git add build.fac input

${FAC:-../../fac} --tupfile Tupfile

grep a awesome
grep 3 data
grep 3 great
grep silliness foo

cat Tupfile

${FAC:-../../fac} -c

ls -lh

tup init
tup

ls -lh
grep a awesome
grep 3 data
grep 3 great
grep silliness foo

exit 0
