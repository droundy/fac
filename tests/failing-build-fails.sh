#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| false
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
grep 'build failed: foobar' loon.out

cat > top.loon <<EOF
| echo good > foobar
> foobar
EOF

if ../../loon > loon.out; then
    cat loon.out
    echo good
else
    cat loon.out
    echo This should have passed
    exit 1
fi

exit 0
