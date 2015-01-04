#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.bilge <<EOF
| sh script.sh
> foobar
EOF

cat > script.sh <<EOF
cat foo bar > foobar
EOF

../../bilge

grep foo foobar
grep bar foobar

sleep 1
echo baz > bar
../../bilge

grep foo foobar
grep baz foobar

cat > script.sh <<EOF
cat foo > foobar
EOF

../../bilge

cat foobar
grep foo foobar
if grep baz foobar; then
    echo It does have baz
    exit 1
else
    echo There is no baz
fi

../../bilge > bilge.out
cat bilge.out
if grep dirty bilge.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

sleep 1
touch bar
../../bilge > bilge.out
cat bilge.out
if grep dirty bilge.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
