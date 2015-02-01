all: fac

fac.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c fac.c

files.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c files.c

targets.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

clean.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c clean.c

new-build.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c new-build.c

git.o :
	gcc -Wall -Werror -O2 -std=c11 -g -c git.c

lib/listset.o :
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

lib/iterablehash.o :
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c iterablehash.c

lib/arrayset.o :
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

lib/syscalls.h : lib/get_syscalls.py
	python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

lib/bigbrother.o : lib/syscalls.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c bigbrother.c

fac : fac.o files.o targets.o clean.o new-build.o git.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpopt -lprofiler -o fac fac.o files.o targets.o clean.o new-build.o git.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

