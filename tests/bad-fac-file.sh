#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my.fac <<EOF
| echo foo > foo

|xThis is a bug
EOF

git init
git add my.fac

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail no rule to build bar
  exit 1
fi
cat fac.err

grep 'error:.my.fac.3. Second' fac.err


cat > my.fac <<EOF
C cache

| echo foo > foo
EOF

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail
  exit 1
fi
cat fac.err

grep 'error: my.fac:1: .C. cache lines' fac.err


cat > my.fac <<EOF

< cache

| echo foo > foo
EOF

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail
  exit 1
fi
cat fac.err

grep 'error: my.fac:2: .<. input lines' fac.err


cat > my.fac <<EOF

> cache

| echo foo > foo
EOF

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail
  exit 1
fi
cat fac.err

grep 'error: my.fac:2: .>. output lines' fac.err


cat > my.fac <<EOF

c cache

| echo foo > foo
EOF

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail
  exit 1
fi
cat fac.err

grep 'error: my.fac:2: .c. cache lines' fac.err

chmod a-r my.fac
ls -l my.fac

if ../../fac 2> fac.err; then
  cat fac.err
  echo build should fail
  exit 1
fi
cat fac.err

grep 'error: unable to open file' fac.err

exit 0
