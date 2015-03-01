#!/bin/sh

set -ev

INC=-I/usr/local/include

(clang $INC -std=c11 -c fac.c)

(clang $INC -std=c11 -c files.c)

(clang $INC -std=c11 -c targets.c)

(clang $INC -std=c11 -c clean.c)

(clang $INC -std=c11 -c new-build.c)

(clang $INC -std=c11 -c git.c)

(clang $INC -std=c11 -c environ.c)

(cd lib && clang $INC -std=c11 -c listset.c)

(cd lib && clang $INC -std=c11 -c iterablehash.c)

(cd lib && clang $INC -std=c11 -c bigbrother.c)

(cd lib && clang $INC -std=c11 -c sha1.c)

(cd lib && clang $INC -std=c11 -c hashset.c)

(clang -L/usr/local/lib -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/bigbrother.o lib/sha1.o lib/hashset.o)

