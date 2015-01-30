#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| cat foocache && echo baz > baz
> baz
EOF

echo stupid > foocache

git init
git add top.bilge

../../bilge

sleep 1
echo please say this > foocache

../../bilge --show-output > bilge.out
cat bilge.out

grep 'please say this' bilge.out

../../bilge --show-output > bilge.out
cat bilge.out

if grep 'please say this' bilge.out; then
    echo we rebuilt when we should not have done so
    exit 1
fi

cat > top.bilge <<EOF
| cat foocache && echo baz > baz
> baz
c cache
EOF

echo please do not say this > foocache

../../bilge --show-output > bilge.out
cat bilge.out

if grep 'please do not say this' bilge.out; then
    echo this was not treated as a cache

    cat top.bilge.done

    exit 1
fi

exit 0
