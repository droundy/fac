#!/bin/sh

set -ev

(cd bigbro && python3 syscalls/darwin.py > syscalls/darwin.h)

(cd bigbro && python3 syscalls/freebsd.py > syscalls/freebsd.h)

(cd bigbro && python3 syscalls/linux.py > syscalls/linux.h)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o bigbro/bigbro-linux.o -c bigbro/bigbro-linux.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o clean.o -c clean.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o environ.o -c environ.c)

(python3 generate-version-header.py > version-identifier.h)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o fac.o -c fac.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o files.o -c files.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o git.o -c git.c)

(cd lib && gcc ${CFLAGS-} -Ibigbro -std=c99 -o intmap.o -c intmap.c)

(cd lib && gcc ${CFLAGS-} -Ibigbro -std=c99 -o iterablehash.o -c iterablehash.c)

(cd lib && gcc ${CFLAGS-} -Ibigbro -std=c99 -o listset.o -c listset.c)

(cd lib && gcc ${CFLAGS-} -Ibigbro -std=c99 -o sha1.o -c sha1.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o mkdir.o -c mkdir.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o new-build.o -c new-build.c)

(gcc ${CFLAGS-} -Ibigbro -std=c99 -o targets.o -c targets.c)

(gcc -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o mkdir.o bigbro/bigbro-linux.o lib/listset.o lib/iterablehash.o lib/intmap.o lib/sha1.o ${LDFLAGS-} -lpopt -lpthread -lm)

