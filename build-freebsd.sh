#!/bin/sh

set -ev

(clang -I/usr/local/include -std=c11 -c fac.c)

(clang -I/usr/local/include -std=c11 -c files.c)

(clang -I/usr/local/include -std=c11 -c targets.c)

(clang -I/usr/local/include -std=c11 -c clean.c)

(clang -I/usr/local/include -std=c11 -c new-build.c)

(clang -I/usr/local/include -std=c11 -c git.c)

(clang -I/usr/local/include -std=c11 -c environ.c)

(cd lib && clang -I/usr/local/include -std=c11 -c listset.c)

(cd lib && clang -I/usr/local/include -std=c11 -c iterablehash.c)

(cd lib && clang -I/usr/local/include -std=c11 -c sha1.c)

(cd lib && clang -I/usr/local/include -std=c11 -c hashset.c)

(cd lib && clang -I/usr/local/include -std=c11 -c posixmodel.c)

(python2 lib/linux-syscalls.py > lib/linux-syscalls.h)

(python2 lib/freebsd-syscalls.py > lib/freebsd-syscalls.h)

(cd lib && clang -I/usr/local/include -std=c11 -c bigbrotheralt.c)

(clang -L/usr/local/lib -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrotheralt.o)

