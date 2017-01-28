#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

export GIT_DIR=`pwd`/git
export GIT_WORK_TREE=`pwd`/working

mkdir git
mkdir working
cd working

cat > top.fac <<EOF
| echo good stuff > foo
EOF

git init
git add top.fac

git rev-parse --show-toplevel
git rev-parse --git-dir

../../../fac

grep 'good stuff' foo

exit 0
