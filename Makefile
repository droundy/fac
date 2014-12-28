all: fileaccesses bilge

fileaccesses: fileaccesses.c listset.c bigbrother.c bigbrother.h listset.h syscalls.h
	gcc -g -O2 -Wall -o fileaccesses fileaccesses.c listset.c bigbrother.c

syscalls.h: get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@

bilge: bilge.c
	gcc -g -o bilge bilge.c
