#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir
cat > top.bilge <<EOF
| sleep 2 && echo foo > foo
> foo

| sleep 2 && echo bar > bar
> bar

| cat foo bar > foobar
> foobar
< foo
< bar
EOF

/usr/bin/time -f '%e' ../../bilge 2> bilge.time

grep foo foobar
grep bar foobar

cat bilge.time
perl -e 'if ($ARGV[0] > 3.5) { print "FAIL: $ARGV[0] too big\n"; exit 1; }' `cat bilge.time`

exit 0
