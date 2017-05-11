#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir repo
cd repo

git init

cat > top.fac <<EOF
| echo hello > hello
> hello

| cat foo.sh > new.fac
> new.fac

EOF

cat > foo.sh <<EOF
| echo goodbye > goodbye
EOF

git add top.fac foo.sh

${FAC:-../../../fac} -v

grep hello hello
grep goodbye goodbye

cd ..
mv repo silly-name
cd silly-name

# I have seen a bug in which renaming a directory (actually copying it
# using cp -a) caused fac to believe there were two rules that would
# produce the same file, because the .fac.tum files had the old
# absolute path name.  I have not yet been able to reproduce this bug
# here.  :( I'm leaving this test in case I see the bug again, so I
# can easily slot in an improvement here.

echo foo > hello

${FAC:-../../../fac} -v

grep hello hello
grep goodbye goodbye

echo it all worked

exit 0
