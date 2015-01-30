#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.loon <<EOF
| echo foo > foo
> foo

| cat foo bar baz > foobar
> foobar

| echo bar > bar
> bar

EOF

git init
git add top.loon

if ../../loon > loon.out; then
    echo This should have failed
    cat loon.out
    exit 1
fi
cat loon.out

grep foo foo
grep bar bar

cat > top.loon <<EOF
| echo foo > foo
> foo

| cat foo bar > foobar
> foobar

| echo bar > bar
> bar

EOF

../../loon

grep foo foo
grep bar bar
grep foo foobar
grep bar foobar

exit 0
