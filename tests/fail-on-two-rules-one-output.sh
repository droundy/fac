#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo good >> foobar && echo nice > nice
> foobar

| echo bad >> foobar && echo mean > mean
> foobar
EOF

git init
git add top.fac

if ${FAC:-../../fac} &> fac.out; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out

grep 'two' fac.out

cat > top.fac <<EOF
| echo good >> foobar && echo nice > nice

| echo bad >> foobar && echo mean > mean
EOF

if ${FAC:-../../fac} &> fac.out; then
    cat fac.out
    echo This should not have passed
    exit 1
fi
cat fac.out

grep 'two' fac.out

cat > top.fac <<EOF
| echo good >> foobar && echo nice > nice
c bar

| echo bad >> foobar && echo mean > mean
c bar
EOF

${FAC:-../../fac}

exit 0
