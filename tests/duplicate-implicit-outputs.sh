#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.bilge <<EOF
| echo foo > foobar && echo baz > baz
> baz

| echo bar > foobar && echo foo > foo
> foo
EOF

git init
git add top.bilge

if ../../bilge > bilge.out 2>&1; then
    cat bilge.out
    echo This should not have passed
    exit 1
fi
cat bilge.out
grep 'same output' bilge.out

exit 0
