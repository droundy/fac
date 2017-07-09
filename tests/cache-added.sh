#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo foo > cache && echo baz > baz

| echo foo > cache && echo bar > bar
EOF

echo good > .cache-read

git init
git add top.fac

if ${FAC:-../../fac}; then
    echo should have failed
    exit 1
fi

cat > top.fac <<EOF
| echo foo > cache && echo baz > baz
c cache

| echo foo > cache && echo bar > bar
c cache
EOF

${FAC:-../../fac};

grep baz baz
grep bar bar

cat > top.fac <<EOF
| echo foo > cache && echo baz > baz
c cache

| echo foo > cache && echo bar > bar
c cache

| cat input > output
EOF

echo hello > input

if ${FAC:-../../fac}; then
    echo should have failed
    exit 1
fi

cat > top.fac <<EOF
| echo foo > cache && echo baz > baz
c cache

| echo foo > cache && echo bar > bar
c cache

| cat input > output
C input
EOF

${FAC:-../../fac};

grep baz baz
grep bar bar
grep hello output

exit 0
