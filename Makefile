all: lib/fileaccesses bilge

lib/fileaccesses: lib/fileaccesses.c lib/listset.c lib/bigbrother.c lib/arrayset.c \
                      lib/syscalls.h lib/listset.h lib/bigbrother.h lib/arrayset.h
	cd lib && gcc -g -O2 -Wall -std=c11 -o fileaccesses fileaccesses.c listset.c bigbrother.c arrayset.c

lib/syscalls.h: lib/get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@

bilge: bilge.c bilge.h targets.c files.c build.c \
       lib/listset.c lib/bigbrother.c lib/syscalls.h lib/arrayset.c
	gcc -lpthread -lpopt -Wall -std=c11 -g -o bilge bilge.c targets.c files.c build.c lib/listset.c lib/bigbrother.c lib/arrayset.c
