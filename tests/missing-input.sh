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

if ../../bilge > bilge.out 2>&1; then
    cat bilge.out
    echo Bilge was okay.  That is not good.
    exit 1
else
    cat bilge.out
    echo Bilge failed as it ought.
fi

if grep 'build failed' bilge.out; then
    echo we should not have attempted a build in the first place
    exit 1
fi

grep 'cannot build' bilge.out | grep baz
grep 'cannot build' bilge.out | grep bar

exit 0
