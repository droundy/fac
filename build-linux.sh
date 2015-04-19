#!/bin/sh

set -ev

(gcc -std=c11 -o clean.o -c clean.c)

(gcc -std=c11 -o environ.o -c environ.c)

(python3 generate-version-header.py > version-identifier.h)

(gcc -std=c11 -o fac.o -c fac.c)

(gcc -std=c11 -o files.o -c files.c)

(gcc -std=c11 -o git.o -c git.c)

(python2 lib/freebsd-syscalls.py > lib/freebsd-syscalls.h)

(python2 lib/linux-syscalls.py > lib/linux-syscalls.h)

(cd lib && gcc -std=c11 -o bigbrotheralt.o -c bigbrotheralt.c)

(cd lib && gcc -std=c11 -o hashset.o -c hashset.c)

(cd lib && gcc -std=c11 -o iterablehash.o -c iterablehash.c)

(cd lib && gcc -std=c11 -o listset.o -c listset.c)

(cd lib && gcc -std=c11 -o posixmodel.o -c posixmodel.c)

(cd lib && gcc -std=c11 -o sha1.o -c sha1.c)

(gcc -std=c11 -o new-build.o -c new-build.c)

(gcc -std=c11 -o targets.o -c targets.c)

(gcc -lpopt -lpthread -lm -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrotheralt.o)

