#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo good > foobar
> foobar

| echo bad > foobar
> foobar
EOF

git init
git add top.fac

if ../../fac > fac.out; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out

exit 0
