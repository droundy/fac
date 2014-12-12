fileaccesses: fileaccesses.c syscalls.h
	gcc -o fileaccesses fileaccesses.c

syscalls.h: get_syscalls.py
	python $< /usr/src/linux-headers-3.2.0-4-common > $@
