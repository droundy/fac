#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir1
mkdir subdir2
ln -s subdir1 subdir3

cat > top.fac <<EOF
| cat input > foobar

| cat foobar > bazbar
< foobar

| cat subdir1/input > noggin

| cat subdir2/foo > foo
< subdir2/foo

| cat subdir3/foo > bar
< subdir3/foo

| cat subdir3/input > test

EOF

cat > subdir1/.fac <<EOF
| cat input > foo
EOF

cat > subdir2/.fac <<EOF
| cat ../input > foo
< ../input
EOF

echo hello > actual_input
ln -sf actual_input input
echo goodbye > subdir1/input
echo testing > subdir2/input

git init
git add top.fac subdir1/.fac subdir2/.fac subdir1/input input actual_input subdir3 subdir2/input

${FAC:-../../fac} -v

cat top.fac.tum

ls -lhR

grep goodbye noggin
grep goodbye subdir3/foo
grep hello subdir2/foo
grep goodbye subdir1/foo
grep hello bazbar
grep hello foobar
grep goodbye bar
grep goodbye test

echo HELLO > new_input
git add new_input
ln -sf new_input input

${FAC:-../../fac} -v

grep goodbye noggin
grep goodbye subdir3/foo
grep HELLO subdir2/foo
grep goodbye subdir1/foo
grep HELLO bazbar
grep HELLO foobar
grep goodbye bar
grep goodbye test

sleep 2
rm subdir3
ln -s subdir2 subdir3
ls -lh subdir3
grep HELLO subdir3/foo

${FAC:-../../fac} -vvv

grep goodbye noggin
grep HELLO subdir3/foo
grep HELLO subdir2/foo
grep goodbye subdir1/foo
grep HELLO bazbar
grep HELLO foobar
grep HELLO bar
grep testing test

exit 0
