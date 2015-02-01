all: fac

fac.o fac.gcno : fac.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c fac.c

files.o files.gcno : files.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c files.c

targets.o targets.gcno : targets.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c targets.c

clean.o clean.gcno : clean.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c clean.c

new-build.o new-build.gcno : new-build.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h lib/bigbrother.h lib/arrayset.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c new-build.c

git.o git.gcno : git.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c git.c

lib/trie.o lib/trie.gcno : lib/trie.c lib/trie.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c trie.c

lib/listset.o lib/listset.gcno : lib/listset.c lib/listset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c listset.c

lib/iterablehash.o lib/iterablehash.gcno : lib/iterablehash.c lib/iterablehash.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c iterablehash.c

lib/arrayset.o lib/arrayset.gcno : lib/arrayset.c lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c arrayset.c

lib/syscalls.h : lib/get_syscalls.py
	python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

lib/bigbrother.o lib/bigbrother.gcno : lib/syscalls.h lib/bigbrother.c lib/bigbrother.h lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -fprofile-arcs -ftest-coverage -c bigbrother.c

fac : fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpopt -lprofiler -fprofile-arcs -ftest-coverage -o fac fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

