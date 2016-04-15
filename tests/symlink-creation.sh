#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir1
mkdir subdir2
ln -s subdir1 subdir3

cat > top.fac <<EOF
| ln -s foo bar
EOF

echo foo > foo

git init
git add top.fac

../../fac

cat top.fac.tum

grep '>' top.fac.tum | grep bar

grep foo bar

exit 0
