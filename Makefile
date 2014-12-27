all: fileaccesses better

fileaccesses: fileaccesses.c listset.c bigbrother.c bigbrother.h listset.h syscalls.h
	gcc -g -o fileaccesses fileaccesses.c listset.c bigbrother.c

syscalls.h: get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@

better: better.c syscalls.h
	gcc -o better better.c

