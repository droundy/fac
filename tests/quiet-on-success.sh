#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.loon <<EOF
| cat message && echo foo > foo
> foo
EOF

echo I am loud > message

git init
git add top.loon message

../../loon > loon.out 2>&1
cat loon.out

if grep 'I am loud' loon.out; then
    echo build was noisy on success
    exit 1
fi

sleep 1
touch message

../../loon --show-output > loon.out 2>&1
cat loon.out

grep 'I am loud' loon.out

exit 0
