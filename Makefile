all: lib/fileaccesses bilge

lib/fileaccesses: lib/fileaccesses.c lib/listset.c lib/bigbrother.c \
                      lib/syscalls.h lib/listset.h lib/bigbrother.h
	cd lib && gcc -g -O2 -Wall -o fileaccesses fileaccesses.c listset.c bigbrother.c

syscalls.h: get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@

bilge: bilge.c
	gcc -g -o bilge bilge.c
