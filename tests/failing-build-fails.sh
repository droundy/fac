#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| false
> foobar
EOF

git init
git add top.fac

if ${FAC:-../../fac} > fac.out; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out
grep 'build failed: f' fac.out

cat > top.fac <<EOF
| echo good > foobar
> foobar
EOF

if ${FAC:-../../fac} > fac.out; then
    cat fac.out
    echo good
else
    cat fac.out
    echo This should have passed
    exit 1
fi

exit 0
