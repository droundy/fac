#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| sh script.sh
> foobar
EOF

cat > script.sh <<EOF
cat foo bar > foobar
EOF

git init
git add top.fac script.sh foo bar

${FAC:-../../fac}

grep foo foobar
grep bar foobar

sleep 1
echo baz > bar
${FAC:-../../fac}

grep foo foobar
grep baz foobar

cat > script.sh <<EOF
cat foo > foobar
EOF

${FAC:-../../fac}

cat foobar
grep foo foobar
if grep baz foobar; then
    echo It does have baz
    exit 1
else
    echo There is no baz
fi

${FAC:-../../fac} > fac.out
cat fac.out
if grep dirty fac.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

sleep 1
touch bar
${FAC:-../../fac} > fac.out
cat fac.out
if grep dirty fac.out; then
    echo It is dirty, but should not be!!!
    exit 1
else
    echo All is well.
fi

exit 0
