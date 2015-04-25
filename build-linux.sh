#!/bin/sh

set -ev

(${CC-gcc} ${CFLAGS-} -std=c99 -o clean.o -c clean.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -o environ.o -c environ.c)

(python3 generate-version-header.py > version-identifier.h)

(${CC-gcc} ${CFLAGS-} -std=c99 -o fac.o -c fac.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -o files.o -c files.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -o git.o -c git.c)

(python2 lib/freebsd-syscalls.py > lib/freebsd-syscalls.h)

(python2 lib/linux-syscalls.py > lib/linux-syscalls.h)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o bigbrother.o -c bigbrother.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o hashset.o -c hashset.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o intmap.o -c intmap.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o iterablehash.o -c iterablehash.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o listset.o -c listset.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o posixmodel.o -c posixmodel.c)

(cd lib && ${CC-gcc} ${CFLAGS-} -std=c99 -o sha1.o -c sha1.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -o new-build.o -c new-build.c)

(${CC-gcc} ${CFLAGS-} -std=c99 -o targets.o -c targets.c)

(${CC-gcc} -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/intmap.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrother.o ${LDFLAGS-} -lpopt -lpthread -lm)

