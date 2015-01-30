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
| cat message; false
> foo

EOF

echo done > message

git init
git add top.loon message

if ../../loon > loon.out; then
    cat loon.out
    echo this should have failed
    exit 1
fi
cat loon.out

grep done loon.out

exit 0
