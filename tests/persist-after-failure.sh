#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| echo foo > foo
> foo

| cat foo bar baz > foobar
> foobar

| echo bar > bar
> bar

EOF

git init
git add top.bilge

if ../../bilge > bilge.out; then
    echo This should have failed
    cat bilge.out
    exit 1
fi
cat bilge.out

grep foo foo
grep bar bar

cat > top.bilge <<EOF
| echo foo > foo
> foo

| cat foo bar > foobar
> foobar

| echo bar > bar
> bar

EOF

../../bilge

grep foo foo
grep bar bar
grep foo foobar
grep bar foobar

exit 0
