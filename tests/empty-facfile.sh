#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > empty.fac <<EOF
# This fac file has no rules.  That should not be a problem!
EOF

cat > foo.fac <<EOF
| echo foo > foo
EOF

git init
git add foo.fac empty.fac

${FAC:-../../fac}

${FAC:-../../fac} -c

${FAC:-../../fac} --exhaustive

exit 0
