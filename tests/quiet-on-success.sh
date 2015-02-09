#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cat message && echo foo > foo
> foo
EOF

echo I am loud > message

git init
git add top.fac message

../../fac > fac.out 2>&1
cat fac.out

if grep 'I am loud' fac.out; then
    echo build was noisy on success
    exit 1
fi

sleep 1
touch message

../../fac > fac.out 2>&1
cat fac.out

if grep 'I am loud' fac.out; then
    echo build was noisy on success
    exit 1
fi

echo >> message

../../fac --show-output > fac.out 2>&1
cat fac.out

grep 'I am loud' fac.out

exit 0
