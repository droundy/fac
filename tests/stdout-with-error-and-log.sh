#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.fac <<EOF
| cat message; false
> foo

EOF

echo done > message

git init
git add top.fac message

if ../../fac --log-output log > fac.out; then
    cat fac.out
    echo this should have failed
    exit 1
fi
cat fac.out

ls log

cat log/foo

grep done fac.out

exit 0
