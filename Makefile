all: fac

fac.o : lib/listset.h lib/sha1.h lib/iterablehash.h fac.h new-build.h fac.c
	gcc -std=c11 -c fac.c

files.o : environ.h lib/listset.h lib/sha1.h fac.h files.c lib/iterablehash.h
	gcc -std=c11 -c files.c

targets.o : lib/iterablehash.h lib/listset.h lib/sha1.h fac.h targets.c
	gcc -std=c11 -c targets.c

clean.o : lib/iterablehash.h lib/listset.h lib/sha1.h fac.h clean.c
	gcc -std=c11 -c clean.c

new-build.o : lib/hashset.h environ.h lib/iterablehash.h lib/listset.h lib/bigbrother.h lib/sha1.h fac.h new-build.c new-build.h
	gcc -std=c11 -c new-build.c

git.o : lib/iterablehash.h lib/listset.h lib/sha1.h fac.h git.c
	gcc -std=c11 -c git.c

environ.o : lib/sha1.h environ.h environ.c
	gcc -std=c11 -c environ.c

lib/listset.o : lib/listset.h lib/listset.c
	cd lib && gcc -std=c11 -c listset.c

lib/iterablehash.o : lib/iterablehash.h lib/iterablehash.c
	cd lib && gcc -std=c11 -c iterablehash.c

lib/sha1.o : lib/sha1.h lib/sha1.c
	cd lib && gcc -std=c11 -c sha1.c

lib/hashset.o : lib/iterablehash.h lib/hashset.h lib/hashset.c
	cd lib && gcc -std=c11 -c hashset.c

lib/posixmodel.o : lib/iterablehash.h lib/posixmodel.c lib/posixmodel.h
	cd lib && gcc -std=c11 -c posixmodel.c

lib/linux-syscalls.h : lib/linux/unistd_64.h lib/linux-syscalls.py lib/linux/unistd_32.h
	python2 lib/linux-syscalls.py > lib/linux-syscalls.h

lib/bigbrother.o : lib/linux-syscalls.h lib/iterablehash.h lib/hashset.h lib/bigbrother.h lib/bigbrother.c
	cd lib && gcc -std=c11 -c bigbrother.c

fac : fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrother.o
	gcc -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrother.o

