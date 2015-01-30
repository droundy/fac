#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.loon <<EOF
| echo foo > foobar && echo baz > baz
> baz

| echo bar > foobar && echo foo > foo
> foo
EOF

git init
git add top.loon

if ../../loon > loon.out 2>&1; then
    cat loon.out
    echo This should not have passed
    exit 1
fi
cat loon.out
grep 'same output' loon.out

exit 0
