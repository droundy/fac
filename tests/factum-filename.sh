#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF

| sh configure.sh
> .fac

EOF

cat > configure.sh <<EOF
echo "| echo foo > foo" > .fac
EOF

git init
git add top.fac configure.sh

${FAC:-../../fac} -vvv

ls -a

cat top.fac.tum

cat .fac.tum

${FAC:-../../fac} -c

ls -a

test ! -e top.fac.tum

test ! -e .fac.tum

exit 0
