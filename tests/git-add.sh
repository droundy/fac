#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cat foo > bar

| cat bar > baz
< bar

| cat input > output
EOF

git init
git add top.fac

echo foo > foo
echo input > input

git ls-files

if git ls-files | grep foo; then
    echo foo should not yet be in git
    exit 1
fi
if git ls-files | grep input; then
    echo foo should not yet be in git
    exit 1
fi

if ../../fac; then
    echo fac should have failed
    exit 1
fi

../../fac --git-add

git ls-files
git ls-files | grep foo
git ls-files | grep input

if git ls-files | grep bar; then
    echo bar should not be added
    exit 1
fi

exit 0
