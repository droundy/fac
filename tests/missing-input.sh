#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.loon <<EOF
| echo foo > foo
> foo

| cat bar > baz
> baz
< bar
EOF

git init
git add top.loon

if ../../loon > loon.out 2>&1; then
    cat loon.out
    echo Bilge was okay.  That is not good.
    exit 1
else
    cat loon.out
    echo Bilge failed as it ought.
fi

grep 'missing file bar' loon.out
grep 'missing file bar' loon.out | grep baz

if grep 'cat: bar' loon.out; then
    echo we should not have attempted this build in the first place
    exit 1
fi

exit 0
