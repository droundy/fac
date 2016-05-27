#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir git
mkdir working
cd working

cat > top.fac <<EOF
| echo good stuff > foo
EOF

git init --separate-git-dir ../git/repo.git
git add top.fac

../../../fac

grep 'good stuff' foo

exit 0
