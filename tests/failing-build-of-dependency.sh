#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

| false
> baz

| cat baz > bar
< baz
> bar
EOF

git init
git add top.fac

if ${FAC:-../../fac} > fac.out 2>&1; then
    cat fac.out
    echo Bilge was okay.  That is not good.
    exit 1
else
    cat fac.out
    echo Bilge failed as it ought.
fi

if grep 'build failed.*bar' fac.out; then
    echo we should not have attempted to build bar in the first place
    exit 1
fi

egrep 'build failed.*(baz|false)' fac.out

exit 0
