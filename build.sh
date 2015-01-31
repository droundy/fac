#!/bin/sh

gcc -Wall -Werror -O2 -std=c11 -g -c fac.c

gcc -Wall -Werror -O2 -std=c11 -g -c files.c

gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

gcc -Wall -Werror -O2 -std=c11 -g -c clean.c

gcc -Wall -Werror -O2 -std=c11 -g -c new-build.c

gcc -Wall -Werror -O2 -std=c11 -g -c git.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c iterablehash.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c bigbrother.c

gcc -lpopt -lprofiler -o fac fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

