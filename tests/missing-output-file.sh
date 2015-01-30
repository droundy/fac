#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo
> bar

| cat bar > ugly
< bar
> ugly
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

if grep 'build failed' fac.out | grep ugly; then
    echo we should not have attempted to build ugly in the first place
    exit 1
fi

grep 'build failed' fac.out | grep bar

exit 0
