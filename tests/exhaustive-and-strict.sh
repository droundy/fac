#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo hello word > foo
EOF

git init
git add top.fac

if ${FAC:-../../fac} --exhaustive --strict; then
    echo we should not be able to be both exhaustive and strict
    exit 1
fi

exit 0
