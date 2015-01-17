#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.bilge <<EOF
| cat message && echo foo > foo
> foo
EOF

echo I am loud > message

../../bilge > bilge.out 2>&1
cat bilge.out

if grep 'I am loud' bilge.out; then
    echo build was noisy on success
    exit 1
fi

sleep 1
touch message

../../bilge --show-output > bilge.out 2>&1
cat bilge.out

grep 'I am loud' bilge.out

exit 0
