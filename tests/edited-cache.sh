#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| cat foocache && echo baz > baz
> baz
EOF

echo stupid > foocache

git init
git add top.fac
git add foocache

${FAC:-../../fac}

sleep 1
echo please say this > foocache

${FAC:-../../fac} --show-output > fac.out
cat fac.out

grep 'please say this' fac.out

${FAC:-../../fac} --show-output > fac.out
cat fac.out

if grep 'please say this' fac.out; then
    echo we rebuilt when we should not have done so
    exit 1
fi

cat > top.fac <<EOF
| cat foocache && echo baz > baz
> baz
c cache
EOF

echo please do not say this > foocache

${FAC:-../../fac} --show-output > fac.out
cat fac.out

if grep 'please do not say this' fac.out; then
    echo this was not treated as a cache

    cat top.fac.tum

    exit 1
fi

exit 0
