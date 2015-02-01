#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.fac <<EOF
| ../spinner.test 2 > foo
> foo
c .gcda

| ../spinner.test 2 > bar
> bar
c .gcda

| cat foo bar > foobar
> foobar
< foo
< bar
EOF

git init
git add top.fac

/usr/bin/time -f '%e' ../../fac -j2 2> fac.time

grep spinning foobar
grep spinning foo
grep spinning bar

cat fac.time
perl -e 'if ($ARGV[0] > 3.5) { print "FAIL: $ARGV[0] too big\n"; exit 1; }' `cat fac.time`

exit 0
