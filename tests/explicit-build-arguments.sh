#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| echo foo > foo
> foo

| cat bar > ugly
< bar
> ugly
EOF

echo hello > bar

git init
git add top.fac bar

${FAC:-../../fac} foo

grep foo foo

if grep hello ugly; then
    echo should not be made
    exit 1
fi

${FAC:-../../fac}

grep hello ugly

echo wrong > foo
echo goodbye > ugly # put wrong value in file

${FAC:-../../fac} foo

grep foo foo
# verify the wrong value is still in the ugly file that we did not
# rebuild
grep goodbye ugly

${FAC:-../../fac}

grep foo foo
grep hello ugly

exit 0
