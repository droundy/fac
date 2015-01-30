#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

| cat bar > baz
> baz
< bar
EOF

git init
git add top.fac

if ../../fac > fac.out 2>&1; then
    cat fac.out
    echo Bilge was okay.  That is not good.
    exit 1
else
    cat fac.out
    echo Bilge failed as it ought.
fi

grep 'missing file bar' fac.out
grep 'missing file bar' fac.out | grep baz

if grep 'cat: bar' fac.out; then
    echo we should not have attempted this build in the first place
    exit 1
fi

exit 0
