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
< foo
< bar
EOF
git add top.fac

if ${FAC:-../../fac} > fac.out; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out

grep 'error: add .*bar.* to git, which is required for .*foobar' fac.out

rm -f foobar
git add bar

${FAC:-../../fac}

exit 0
