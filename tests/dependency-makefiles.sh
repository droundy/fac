#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.fac <<EOF
| gcc -MD -MF .foo.o.dep -c foo.c
M .foo.o.dep

| gcc -o foo foo.o
< foo.o
> foo

| file foo && ./foo > message
< foo
> message
EOF

cat > foo.h <<EOF
const char *message = "hello\n";
EOF
cat > foo.c <<EOF
#include <stdio.h>
#include "foo.h"

int main() {
  printf(message);
  return 0;
}
EOF

git init
git add top.fac foo.c foo.h

${FAC:-../../fac}

grep hello message

cat .foo.o.dep

${FAC:-../../fac} -c

test ! -e .foo.o.dep
test ! -e foo.o
test ! -e foo
test ! -e message

${FAC:-../../fac} --blind

cat > "name with\tspace.h" <<EOF
const char *message = "greetings\n";
EOF
git add "name with\tspace.h"
cat > foo.c <<EOF
#include <stdio.h>
#include "name with\tspace.h"

int main() {
  printf(message);
  return 0;
}
EOF

${FAC:-../../fac}

grep greetings message

cat .foo.o.dep

${FAC:-../../fac}

exit 0
