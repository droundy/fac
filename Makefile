all: lib/fileaccesses bilge

lib/fileaccesses: lib/fileaccesses.c lib/listset.c lib/bigbrother.c \
                      lib/syscalls.h lib/listset.h lib/bigbrother.h
	cd lib && gcc -g -O2 -Wall -o fileaccesses fileaccesses.c listset.c bigbrother.c

lib/syscalls.h: lib/get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@

bilge: bilge.c bilge.h targets.c files.c
	gcc -Wall -std=c11 -g -o bilge bilge.c targets.c files.c
