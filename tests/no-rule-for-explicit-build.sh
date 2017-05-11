#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| echo foo > foo
EOF

git init
git add my.fac

if ${FAC:-../../fac} bar; then
  echo build should fail no rule to build bar
  exit 1
fi

if ${FAC:-../../fac} foo; then
  echo build should fail no rule to build foo
  exit 1
fi

${FAC:-../../fac}

${FAC:-../../fac} foo

grep foo foo

exit 0
