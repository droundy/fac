#!/bin/sh

set -ev

(gcc -std=c11 -c fac.c)

(gcc -std=c11 -c files.c)

(gcc -std=c11 -c targets.c)

(gcc -std=c11 -c clean.c)

(gcc -std=c11 -c new-build.c)

(gcc -std=c11 -c git.c)

(gcc -std=c11 -c environ.c)

(cd lib && gcc -std=c11 -c listset.c)

(cd lib && gcc -std=c11 -c iterablehash.c)

(cd lib && gcc -std=c11 -c arrayset.c)

(python2 lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h)

(cd lib && gcc -std=c11 -c bigbrother.c)

(cd lib && gcc -std=c11 -c sha1.c)

(cd lib && gcc -std=c11 -c hashset.c)

(gcc -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o lib/sha1.o lib/hashset.o)

