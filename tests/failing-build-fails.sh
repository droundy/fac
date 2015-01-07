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

if ../../bilge; then
    echo This should not have passed
    exit 1
fi

cat > top.bilge <<EOF
| echo good > foobar
> foobar
EOF

if ../../bilge; then
    echo good
else
    echo This should have passed
    exit 1
fi

exit 0
