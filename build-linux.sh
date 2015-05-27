#!/bin/sh

set -ev

(if cd bigbro; then git pull; else git clone git://github.com/droundy/bigbro; fi)

(cd bigbro && python3 syscalls/darwin.py > syscalls/darwin.h)

(cd bigbro && python3 syscalls/freebsd.py > syscalls/freebsd.h)

(cd bigbro && python3 syscalls/linux.py > syscalls/linux.h)

(cd bigbro && ${CC-gcc} -Wall -Werror -O2 -std=c99 -g -mtune=native -c bigbro-linux.c)

(cd bigbro && rm -f libbigbro.a && ${AR-ar} rc libbigbro.a bigbro-linux.o && ${RANLIB-ranlib} libbigbro.a)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o clean.o -c clean.c)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o environ.o -c environ.c)

(python3 generate-version-header.py > version-identifier.h)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o fac.o -c fac.c)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o files.o -c files.c)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o git.o -c git.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o intmap.o -c intmap.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o iterablehash.o -c iterablehash.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o listset.o -c listset.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o sha1.o -c sha1.c)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o new-build.o -c new-build.c)

(${CC-gcc} ${CFLAGS-} -Ibigbro -std=c99 -o targets.o -c targets.c)

(${CC-gcc} -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/intmap.o lib/sha1.o ${LDFLAGS-} -Lbigbro -lpopt -lpthread -lm -lbigbro)

