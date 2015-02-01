#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

git init
git add foo

cat > top.fac <<EOF
| cat foo bar > foobar
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

# check that it prints the right file names
grep 'error: bar should be in git for foobar' fac.out

rm -f foobar
git add bar

../../fac

exit 0
