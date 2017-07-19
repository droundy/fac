#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.fac <<EOF
| echo foo > foobar && echo baz > baz
> baz
c foobar

| echo foo > /tmp/junk-$$ && echo baz > bar
> bar

| echo foo >  .cache-me && cat .cache-read > localcache
> localcache
C .cache

| echo bar > foobar && echo foo > foo
> foo
EOF

echo good > .cache-read

git init
git add top.fac

${FAC:-../../fac}

cat top.fac.tum

if egrep '^. \.cache-me' top.fac.tum; then
  echo this file should be ignored
  exit 1
fi
if egrep '^. \.cache-read' top.fac.tum; then
  echo this file should be ignored
  exit 1
fi
grep "foobar" top.fac.tum


cat > top.fac <<EOF
| echo foo > foobar && echo baz > baz
> baz
c foobar

| echo foo > /tmp/junk-$$ && echo baz > bar
> bar

| echo foo >  .cache-me && cat .cache-read > localcache
> localcache
C .cache

| echo bar > foobar && echo foo > foo
> foo
c oobar
EOF

rm foo

${FAC:-../../fac}
cat top.fac.tum


egrep '^. baz' top.fac.tum

if egrep '^. foobar' top.fac.tum; then
  echo this file should be ignored
  exit 1
fi

rm -f /tmp/junk-$$

exit 0
