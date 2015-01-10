#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.bilge <<EOF
| cat message; echo foo > foo
> foo

EOF

echo done > message

../../bilge > bilge.out
cat bilge.out

grep done bilge.out

grep foo foo

exit 0
