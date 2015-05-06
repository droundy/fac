#!/bin/sh

set -ev

(rm -rf bigbro && git clone git://github.com/droundy/bigbro)

(cd bigbro && sh build-linux.sh)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o clean.o -c clean.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o environ.o -c environ.c)

(python3 generate-version-header.py > version-identifier.h)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o fac.o -c fac.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o files.o -c files.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o git.o -c git.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o intmap.o -c intmap.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o iterablehash.o -c iterablehash.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o listset.o -c listset.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o sha1.o -c sha1.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o new-build.o -c new-build.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -Ibigbro -o targets.o -c targets.c)

(${CC-gcc} -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/intmap.o lib/sha1.o ${LDFLAGS-} -lpopt -lpthread -lm -L/home/droundy/src/fac/bigbro -lbigbro)

