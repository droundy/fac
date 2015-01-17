#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

git init
git add foo

cat > top.bilge <<EOF
| cat foo bar > foobar
> foobar
< foo
< bar
EOF

if ../../bilge > bilge.out; then
    cat bilge.out
    echo This should not have passed
    exit 1
fi
cat bilge.out

rm foobar
git add bar

../../bilge

exit 0
