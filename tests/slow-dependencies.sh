#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

# This test illustrates a bug in which we don't wait to see if a file
# had been created by a rule prior to concluding that it should have
# been in git.

cat > top.fac <<EOF
| date > f1 && sleep 1
| date > f2 && sleep 1
| date > f3 && sleep 1
| date > f4 && sleep 1
| date > f5 && sleep 1
| date > f6 && sleep 1
| ls -l > listing
| date > f7 && sleep 1
| date > f8 && sleep 1
| date > f9 && sleep 1
EOF

git init
git add top.fac

${FAC:-../../fac} -j12

exit 0
