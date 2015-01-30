#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.fac <<EOF
| echo foo > foo
> foo

| cat foo bar baz > foobar
> foobar

| echo bar > bar
> bar

EOF

git init
git add top.fac

if ../../fac > fac.out; then
    echo This should have failed
    cat fac.out
    exit 1
fi
cat fac.out

grep foo foo
grep bar bar

cat > top.fac <<EOF
| echo foo > foo
> foo

| cat foo bar > foobar
> foobar

| echo bar > bar
> bar

EOF

../../fac

grep foo foo
grep bar bar
grep foo foobar
grep bar foobar

exit 0
