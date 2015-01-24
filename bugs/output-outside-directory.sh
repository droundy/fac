#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| echo foo > ~/.tmp-trash && echo baz > baz
> baz
EOF

if ../../bilge > bilge.out 2>&1; then
    cat bilge.out
    echo This should not have passed
    exit 1
fi
cat bilge.out

exit 0
