#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| ../spinner.test 2 > foo
> foo

| ../spinner.test 2 > bar
> bar

| cat foo bar > foobar
> foobar
< foo
< bar
EOF

/usr/bin/time -f '%e' ../../bilge -j2 2> bilge.time

grep spinning foobar
grep spinning foo
grep spinning bar

cat bilge.time
perl -e 'if ($ARGV[0] > 3.5) { print "FAIL: $ARGV[0] too big\n"; exit 1; }' `cat bilge.time`

exit 0
