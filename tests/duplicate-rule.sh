#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| echo foo > foobar
> foobar

| echo foo > foobar
> foobar
EOF

git init
git add top.loon

if ../../loon > loon.out 2>&1; then
    cat loon.out
    echo This should not have passed
    exit 1
fi
cat loon.out
grep -i 'duplicate rule' loon.out

exit 0
