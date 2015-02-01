#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

if ../../fac; then
  echo build should fail with no fac file
  exit 1
fi

cat > my.fac <<EOF
| echo foo > foo
EOF

if ../../fac; then
  echo build should fail with fac file not git added
  exit 1
fi

exit 0
