#!/bin/sh

# Relative and symlink paths fail on initial build

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir -p dir1
> dir1

| echo "foo" > dir1/foo
< dir1
> dir1/foo

# aliased symbolic link to dir1 causes failure

| ln -s dir1 sym1
< dir1
> sym1

| cat sym1/foo > foo
< sym1/foo
> foo
EOF

mkdir dir2
cat > dir2/build.fac <<EOF
| cat ../dir1/foo > foo
< ../dir1/foo
> foo
EOF

git init
git add .

# this will fail with:
#   error: add dir2/../dir1/foo to git, which is required for dir2/foo
#   error: add sym1/foo to git, which is required for foo
../../fac

# subsequent attempt will work since directories are available
../../fac

exit 0
