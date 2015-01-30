#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| echo foo > foo
> foo
EOF

git init
git add top.bilge

../../bilge -v > bilge.out
cat bilge.out
if grep dirty bilge.out; then
    echo It is dirty, as it should be.
else
    echo It should not be clean!!!
    exit 1
fi

cat top.bilge.done

../../bilge -v > bilge.out
cat bilge.out
if grep dirty bilge.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
