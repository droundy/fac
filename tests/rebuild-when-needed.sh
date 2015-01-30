#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| cat foo > bar
> bar
< foo

| cat input > foo
> foo

EOF

echo good > input

git init
git add top.bilge input

../../bilge -v

grep good foo
grep good bar

echo newer > input

../../bilge

grep newer foo
grep newer bar

exit 0
