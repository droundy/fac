#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| cat foo > bar
> bar
< foo

| cat input > foo
> foo

EOF

echo good > input

git init
git add top.fac input

../../fac -v

grep good foo
grep good bar

echo newer > input

../../fac

grep newer foo
grep newer bar

exit 0
