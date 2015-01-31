all: fac

fac.o : fac.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h
	gcc -Wall -Werror -O2 -std=c11 -g -c fac.c

files.o : files.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c files.c

targets.o : targets.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

clean.o : clean.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c clean.c

new-build.o : new-build.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h lib/bigbrother.h lib/arrayset.h
	gcc -Wall -Werror -O2 -std=c11 -g -c new-build.c

git.o : git.c fac.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c git.c

lib/trie.o : lib/trie.c lib/trie.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

lib/listset.o : lib/listset.c lib/listset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

lib/iterablehash.o : lib/iterablehash.c lib/iterablehash.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c iterablehash.c

lib/arrayset.o : lib/arrayset.c lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

lib/syscalls.h : lib/get_syscalls.py
	python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

lib/bigbrother.o : lib/syscalls.h lib/bigbrother.c lib/bigbrother.h lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c bigbrother.c

fac : fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpopt -lprofiler -o fac fac.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

