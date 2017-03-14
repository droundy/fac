#!/bin/sh

set -ev

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o arguments.o -c arguments.c)

(if cd bigbro; then git pull; else git clone https://github.com/droundy/bigbro; fi)

(cd bigbro && python3 syscalls/linux.py > syscalls/linux.h)

(cd bigbro && gcc $CFLAGS -O2 -Wall -Werror -std=c99 -g -mtune=native -fpic -c bigbro-linux.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o build.o -c build.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o clean-all.o -c clean-all.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o environ.o -c environ.c)

(python generate-version-header.py > version-identifier.h)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o fac.o -c fac.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o files.o -c files.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o git.o -c git.c)

(cd lib && gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o iterablehash.o -c iterablehash.c)

(cd lib && gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o listset.o -c listset.c)

(cd lib && gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o sha1.o -c sha1.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o mkdir.o -c mkdir.c)

(gcc ${CFLAGS} -Ibigbro -std=c99 -flto -o targets.o -c targets.c)

(gcc -o fac fac.o files.o targets.o clean-all.o build.o git.o environ.o mkdir.o arguments.o bigbro/bigbro-linux.o lib/listset.o lib/iterablehash.o lib/sha1.o ${LDFLAGS-} -lpthread -lm -flto)

rm -rf bigbro
