#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF

| echo working && echo foo > foo
> foo

EOF

git init
git add top.fac

../../fac --log-output log

grep foo foo

ls -l log

grep working log/foo

exit 0
