#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.bilge <<EOF
| echo foo > foo
> foo

| cat bar > baz
> baz
< bar
EOF

git init
git add top.bilge

if ../../bilge > bilge.out 2>&1; then
    cat bilge.out
    echo Bilge was okay.  That is not good.
    exit 1
else
    cat bilge.out
    echo Bilge failed as it ought.
fi

grep 'missing file bar' bilge.out
grep 'missing file bar' bilge.out | grep baz

if grep 'cat: bar' bilge.out; then
    echo we should not have attempted this build in the first place
    exit 1
fi

exit 0
