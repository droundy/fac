#!/bin/sh

# This test is for issue #ABC on github.

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir foo

| echo welcome > foo/bar
< foo

| mkdir foo/subdir
< foo

| mkdir foo/subdir/subsub
< foo/subdir

| echo awesome > foo/subdir/subsub/great
< foo/subdir/subsub

| mkdir foo/subdir2
< foo
EOF

git init
git add top.fac

../../fac

pat='^Build succeeded!.*$'
if [[ ! `../../fac -v -j 1` =~ $pat ]]; then
  echo repeat build should have succeeded w/out additional work
  exit 1
fi

exit 0
