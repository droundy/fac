#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo foo > ~/.tmp-trash && echo baz > baz
> baz
EOF

git init
git add top.fac

if ../../fac > fac.out 2>&1; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out

exit 0
