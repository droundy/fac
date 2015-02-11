#!/bin/sh

set -ev

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c fac.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c files.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c targets.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c clean.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c new-build.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c git.c)

(gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c environ.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c listset.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c iterablehash.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c arrayset.c)

(python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c bigbrother.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -std=c99 -g -c sha1.c)

(gcc -lpopt -lpthread -lprofiler -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o lib/sha1.o)

