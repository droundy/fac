#!/bin/sh

set -ev

if ! which ninja; then
    echo there is no ninja
    exit 137
fi

if ! (echo $FAC | grep target) ; then
    echo not using rust
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

${FAC:-../../fac} --ninja build.ninja

grep a awesome
grep 3 data
grep 3 great
grep silliness foo

cat build.ninja

${FAC:-../../fac} -c

ls -lh

ninja

ls -lh
grep a awesome
grep 3 data
grep 3 great
grep silliness foo

exit 0
