#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
* ls > foo
> foo
EOF

echo hello > hello

git init
git add top.fac

${FAC:-../../fac}

cat foo

grep hello foo
grep top.fac foo

${FAC:-../../fac}

cat foo

grep hello foo
grep top.fac foo
grep foo foo

sleep 1

echo goodbye > goodbye

${FAC:-../../fac} -v

cat foo

grep hello foo
grep top.fac foo
grep foo foo
grep goodbye foo
grep top.fac.tum foo

exit 0
