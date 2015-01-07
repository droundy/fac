#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| cat input > foo
> foo

| cat foo > bar
> bar
< foo
EOF

echo good > input

../../bilge

grep good foo
grep good bar

echo newer > input

../../bilge

grep newer foo
grep newer bar

exit 0
