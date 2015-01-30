#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

git init
git add foo

cat > top.loon <<EOF
| cat foo bar > foobar
> foobar
EOF

git init
git add top.loon

if ../../loon > loon.out; then
    cat loon.out
    echo This should not have passed
    exit 1
fi
cat loon.out

rm -f foobar
git add bar

../../loon

exit 0
