#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.loon <<EOF
| cat subdir/foo subdir/bar > foobar
> foobar
< subdir/foo
< subdir/bar

EOF

cat > subdir/.loon <<EOF
| echo foo > foo
> foo

| echo bar > bar
> bar

EOF

git init
git add top.loon subdir/.loon

../../loon -v

grep foo foobar
grep bar foobar

exit 0
