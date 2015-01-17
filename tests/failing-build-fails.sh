#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| false
> foobar
EOF

if ../../bilge > bilge.out; then
    cat bilge.out
    echo This should not have passed
    exit 1
fi
cat bilge.out
grep 'build failed: foobar' bilge.out

cat > top.bilge <<EOF
| echo good > foobar
> foobar
EOF

if ../../bilge > bilge.out; then
    cat bilge.out
    echo good
else
    cat bilge.out
    echo This should have passed
    exit 1
fi

exit 0
