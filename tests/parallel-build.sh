#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.loon <<EOF
| ../spinner.test 2 > foo
> foo

| ../spinner.test 2 > bar
> bar

| cat foo bar > foobar
> foobar
< foo
< bar
EOF

git init
git add top.loon

/usr/bin/time -f '%e' ../../loon -j2 2> loon.time

grep spinning foobar
grep spinning foo
grep spinning bar

cat loon.time
perl -e 'if ($ARGV[0] > 3.5) { print "FAIL: $ARGV[0] too big\n"; exit 1; }' `cat loon.time`

exit 0
