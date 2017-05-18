#!/bin/sh

# This test is for issue #ABC on github.

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# The build rules below would be more robust if we used mkdir -p, but
# that would defeat the point, which is to ensure that we aren't
# rerunning these rules unnecessarily.  So we use just a plain mkdir,
# which will fail if it is run a second time.

cat > top.fac <<EOF
| mkdir foo

| echo welcome > foo/bar
< foo

| mkdir foo/subdir
< foo

| mkdir foo/subdir/subsub
< foo/subdir

| sleep 1 && echo awesome > foo/subdir/subsub/great
< foo/subdir/subsub

| sleep 1 && mkdir foo/subdir2
< foo
EOF

git init
git add top.fac

${FAC:-../../fac}

grep foo top.fac.tum

cat top.fac.tum

${FAC:-../../fac} -v -j 1

exit 0
