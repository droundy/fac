#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

echo foo > foo
echo bar > bar

cat > top.loon <<EOF
| echo foo > foobar && echo baz > baz
> baz
c foobar

| echo foo > /tmp/junk && echo baz > bar
> bar
C /tmp

| echo foo >  .cache-me && cat .cache-read > localcache
> localcache
C .cache

| echo bar > foobar && echo foo > foo
> foo
EOF

echo good > .cache-read

git init
git add top.loon

../../loon

cat top.loon.done

if grep "$0.dir/.cache-me" top.loon.done; then
  echo this file should be ignored
  exit 1
fi
if grep "$0.dir/.cache-read" top.loon.done; then
  echo this file should be ignored
  exit 1
fi
grep "$0.dir/foobar" top.loon.done


cat > top.loon <<EOF
| echo foo > foobar && echo baz > baz
> baz
c foobar

| echo foo > /tmp/junk && echo baz > bar
> bar
C /tmp

| echo foo >  .cache-me && cat .cache-read > localcache
> localcache
C .cache

| echo bar > foobar && echo foo > foo
> foo
c oobar
EOF

rm foo

../../loon
cat top.loon.done

if grep "$0.dir/foobar" top.loon.done; then
  echo this file should be ignored
  exit 1
fi

exit 0
