#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

| echo bar > bar

| echo zoo > zoo

| echo aaa > aaa

| echo bbb > bbb

| echo ccc > ccc

| echo ddd > ddd

| cat foo bar zoo aaa bbb ccc ddd > all
< foo
< bar
< zoo
< aaa
< bbb
< ccc
< ddd

EOF

git init
git add top.fac

../../fac -v

grep foo foo
grep bar bar
grep zoo zoo

grep foo all
grep bar all
grep zoo all

grep aaa all
grep bbb all
grep ccc all
grep ddd all

exit 0
