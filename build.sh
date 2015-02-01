#!/bin/sh

set -ev

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c fac.c)

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c files.c)

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c targets.c)

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c clean.c)

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c new-build.c)

(gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c git.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c trie.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c listset.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c iterablehash.c)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c arrayset.c)

(python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h)

(cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c bigbrother.c)

(gcc -lpopt -lprofiler -fprofile-arcs -ftest-coverage -o fac fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o)

