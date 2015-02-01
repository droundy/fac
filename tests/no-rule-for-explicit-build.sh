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

if ../../fac bar; then
  echo build should fail no rule to build bar
  exit 1
fi

if ../../fac foo; then
  echo build should fail no rule to build foo
  exit 1
fi

../../fac

../../fac foo

grep foo foo

exit 0
