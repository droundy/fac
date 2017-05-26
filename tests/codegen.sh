#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| mkdir -p main.out
> main.out

| mkdir -p lib.out
> lib.out

| python main.py > main.out/main.fac
< main.out
< main
> main.out/main.fac

| python lib.py > lib.out/lib.fac
< lib.out
< lib
> lib.out/lib.fac
EOF

cat > lib.py <<EOF
import sys, glob, re

srcs = []
objs = []

for src in glob.glob('lib/*.c'):
    base = re.split('/|\.', src)[-2]
    srcs.append(src)
    objs.append("%s.o" % base)

for src, obj in zip(srcs, objs):
    print('''| gcc -I../inc -c ../%s -o %s
< ../%s
> %s
''' % (src, obj, src, obj))

print('''| ar rcs lib.a %s
< %s
> lib.a
''' % (' '.join(objs), '\n< '.join(objs)))
EOF
cat > main.py <<EOF
import sys, glob, re

srcs = []
objs = []

for src in glob.glob('main/*.c'):
    base = re.split('/|\.', src)[-2]
    srcs.append(src)
    objs.append("%s.o" % base)

for src, obj in zip(srcs, objs):
    print('''| gcc -I../inc -c ../%s -o %s
< ../%s
> %s
''' % (src, obj, src, obj))

print('''| gcc -o main %s ../lib.out/lib.a
< %s
< ../lib.out/lib.a
> main
''' % (' '.join(objs), '\n< '.join(objs)))
EOF

mkdir inc
cat > inc/lib.h <<EOF
extern int foo;
EOF

mkdir lib
cat > lib/foo.c <<EOF
#include <lib.h>
int foo = 1;
EOF

mkdir main
cat > main/main.c <<EOF
#include <stdio.h>
#include <lib.h>
int main() {
  printf("success\n");
  return 0;
}
EOF

git init
git add .

${FAC:-../../fac}
${FAC:-../../fac}

cat >> inc/lib.h <<EOF
extern int bar;
EOF

cat > lib/bar.c <<EOF
#include <lib.h>
int bar = 2;
EOF
git add lib/bar.c

${FAC:-../../fac}
${FAC:-../../fac}

main.out/main

exit 0
