#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foobar && echo baz > baz
> baz

| echo bar > foobar && echo foo > foo
> foo
EOF

git init
git add top.fac

if ../../fac > fac.out 2>&1; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out
grep 'same output' fac.out

exit 0
