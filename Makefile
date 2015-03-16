all: fac

fac.o : fac.h lib/sha1.h lib/listset.h fac.c lib/iterablehash.h new-build.h
	gcc -std=c11 -c fac.c

files.o : environ.h fac.h lib/sha1.h lib/listset.h lib/iterablehash.h files.c
	gcc -std=c11 -c files.c

targets.o : fac.h targets.c lib/sha1.h lib/listset.h lib/iterablehash.h
	gcc -std=c11 -c targets.c

clean.o : fac.h lib/sha1.h lib/listset.h lib/iterablehash.h clean.c
	gcc -std=c11 -c clean.c

new-build.o : lib/sha1.h new-build.c environ.h fac.h lib/bigbrother.h lib/listset.h lib/iterablehash.h lib/hashset.h new-build.h
	gcc -std=c11 -c new-build.c

git.o : fac.h lib/sha1.h lib/listset.h lib/iterablehash.h git.c
	gcc -std=c11 -c git.c

environ.o : environ.c environ.h lib/sha1.h
	gcc -std=c11 -c environ.c

lib/listset.o : lib/listset.h lib/listset.c
	cd lib && gcc -std=c11 -c listset.c

lib/iterablehash.o : lib/iterablehash.h lib/iterablehash.c
	cd lib && gcc -std=c11 -c iterablehash.c

lib/sha1.o : lib/sha1.h lib/sha1.c
	cd lib && gcc -std=c11 -c sha1.c

lib/hashset.o : lib/hashset.h lib/iterablehash.h lib/hashset.c
	cd lib && gcc -std=c11 -c hashset.c

lib/posixmodel.o : lib/iterablehash.h lib/hashset.h lib/posixmodel.h lib/posixmodel.c
	cd lib && gcc -std=c11 -c posixmodel.c

lib/linux-syscalls.h : lib/linux/unistd_64.h lib/linux-syscalls.py lib/linux/unistd_32.h lib/gentables.py
	python2 lib/linux-syscalls.py > lib/linux-syscalls.h

lib/bigbrotheralt.o : lib/linux-syscalls.h lib/bigbrotheralt.c lib/iterablehash.h lib/hashset.h lib/bigbrother.h lib/posixmodel.h
	cd lib && gcc -std=c11 -c bigbrotheralt.c

fac : fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrotheralt.o
	gcc -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/sha1.o lib/hashset.o lib/posixmodel.o lib/bigbrotheralt.o

