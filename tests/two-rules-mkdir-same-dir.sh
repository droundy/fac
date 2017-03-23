#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir -p foo && echo hello > foo/hello

| mkdir -p foo && echo goodbye > foo/goodbye
EOF

git init
git add top.fac

../../fac

rm -rf foo

../../fac foo/goodbye

rm -rf foo

../../fac foo/hello

exit 0
