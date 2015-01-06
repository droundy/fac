#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

# This test doesn't pass, but really should test that when we give a
# false output file *something* smart should happen, such as a failed
# build.

cat > top.bilge <<EOF
| echo foo > bar
> foobar
> bar
EOF

../../bilge > bilge.out
cat bilge.out

grep foo bar

if grep dirty bilge.out; then
    echo It is dirty, as it should be!
else
    echo It is clean, which is bad.
    exit 1
fi

../../bilge > bilge.out
cat bilge.out

grep foo bar

if grep dirty bilge.out; then
    echo It is dirty, as it should NOT be!
    exit 1
else
    echo It is clean, which is good.
fi

exit 0
