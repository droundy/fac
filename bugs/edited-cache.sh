#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| cat foocache && echo baz > baz
> baz
EOF

echo stupid > foocache

git init
git add top.loon

../../loon

sleep 1
echo please say this > foocache

../../loon --show-output > loon.out
cat loon.out

grep 'please say this' loon.out

../../loon --show-output > loon.out
cat loon.out

if grep 'please say this' loon.out; then
    echo we rebuilt when we should not have done so
    exit 1
fi

cat > top.loon <<EOF
| cat foocache && echo baz > baz
> baz
c cache
EOF

echo please do not say this > foocache

../../loon --show-output > loon.out
cat loon.out

if grep 'please do not say this' loon.out; then
    echo this was not treated as a cache

    cat top.loon.done

    exit 1
fi

exit 0
