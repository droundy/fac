#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| sh script.sh
> foobar
EOF

cat > script.sh <<EOF
cat foo bar > foobar
EOF

git init
git add top.loon script.sh foo bar

../../loon

grep foo foobar
grep bar foobar

sleep 1
echo baz > bar
../../loon

grep foo foobar
grep baz foobar

cat > script.sh <<EOF
cat foo > foobar
EOF

../../loon

cat foobar
grep foo foobar
if grep baz foobar; then
    echo It does have baz
    exit 1
else
    echo There is no baz
fi

../../loon > loon.out
cat loon.out
if grep dirty loon.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

sleep 1
touch bar
../../loon > loon.out
cat loon.out
if grep dirty loon.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
