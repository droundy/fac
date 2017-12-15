#!/bin/sh

set -ev

echo $0

# This test shows that if a file is created by a rule that generates a
# facfile, then any rules in that facfile will implicitly depend on
# the file being created.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cp facfile facfile.fac && echo good > good
> facfile.fac
EOF

cat > facfile <<EOF
| cp good excellent
< good
EOF

git init
git add top.fac facfile

${FAC:-../../fac} --strict

grep good good
grep good excellent

exit 0
