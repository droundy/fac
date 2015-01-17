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
| cat message; false
> foo

EOF

echo done > message

if ../../bilge > bilge.out; then
    cat bilge.out
    echo this should have failed
    exit 1
fi
cat bilge.out

grep done bilge.out

exit 0
