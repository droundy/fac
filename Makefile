all: fac

fac.o : fac.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h new-build.h
	gcc -std=c11 -c fac.c

files.o : files.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h environ.h
	gcc -std=c11 -c files.c

targets.o : targets.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h
	gcc -std=c11 -c targets.c

clean.o : clean.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h
	gcc -std=c11 -c clean.c

new-build.o : new-build.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h new-build.h environ.h lib/bigbrother.h lib/arrayset.h lib/hashset.h
	gcc -std=c11 -c new-build.c

git.o : git.c fac.h lib/sha1.h lib/listset.h lib/iterablehash.h
	gcc -std=c11 -c git.c

environ.o : environ.c environ.h lib/sha1.h
	gcc -std=c11 -c environ.c

lib/listset.o : lib/listset.c lib/listset.h
	cd lib && gcc -std=c11 -c listset.c

lib/iterablehash.o : lib/iterablehash.c lib/iterablehash.h
	cd lib && gcc -std=c11 -c iterablehash.c

lib/arrayset.o : lib/arrayset.c lib/arrayset.h
	cd lib && gcc -std=c11 -c arrayset.c

lib/bigbrother.o : lib/syscalls.h lib/bigbrother.c lib/bigbrother.h lib/arrayset.h lib/hashset.h lib/iterablehash.h
	cd lib && gcc -std=c11 -c bigbrother.c

lib/sha1.o : lib/sha1.c lib/sha1.h
	cd lib && gcc -std=c11 -c sha1.c

lib/hashset.o : lib/hashset.c lib/hashset.h lib/iterablehash.h
	cd lib && gcc -std=c11 -c hashset.c

fac : fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o lib/sha1.o lib/hashset.o
	gcc -lpopt -lpthread -o fac fac.o files.o targets.o clean.o new-build.o git.o environ.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o lib/sha1.o lib/hashset.o

