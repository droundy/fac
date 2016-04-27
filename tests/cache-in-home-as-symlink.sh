#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# create a new "home directory" for testing
mkdir -p realhome/.cache
# now make a symlink to that actual "home directory", and make the
# symlink be our $HOME.
ln -s `pwd`/realhome home
export HOME=`pwd`/home

echo $HOME

mkdir repo
cd repo

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
# This command creates junk in $HOME, which we instruct fac to ignore.
| echo foo >> ~/.cache/trash && echo baz > baz
C ~/.cache/
> baz
EOF

git init
git add top.fac

../../../fac

grep baz baz
grep foo ~/.cache/trash

exit 0
