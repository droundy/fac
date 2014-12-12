#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "syscalls.h"

void do_trace(pid_t child);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s prog args\n", argv[0]);
    exit(1);
  }

  pid_t child = fork();
  if (child == 0) {
    char **args = (char **)malloc(argc*sizeof(char*));
    memcpy(args, argv+1, (argc-1) * sizeof(char*));
    args[argc-1] = NULL;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
  } else {
    do_trace(child);
  }
  return 0;
}

int wait_for_syscall(pid_t child);

long get_syscall_arg(const struct user_regs_struct *regs, int which) {
    switch (which) {
#ifdef __amd64__
    case 0: return regs->rdi;
    case 1: return regs->rsi;
    case 2: return regs->rdx;
    case 3: return regs->r10;
    case 4: return regs->r8;
    case 5: return regs->r9;
#else
    case 0: return regs->ebx;
    case 1: return regs->ecx;
    case 2: return regs->edx;
    case 3: return regs->esi;
    case 4: return regs->edi;
    case 5: return regs->ebp;
#endif
    default: return -1L;
    }
}


void do_trace(pid_t child) {
  int status, syscall, retval;
  waitpid(child, &status, 0);
  ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
  while(1) {
    struct user_regs_struct regs;
    if (wait_for_syscall(child) != 0) break;

    if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
      fprintf(stderr, "ERROR PTRACING!\n");
      exit(1);
    }
    syscall = regs.orig_rax;

    if (syscall == SYS_read) {
      fprintf(stderr, "read(%d) = ", get_syscall_arg(&regs, 0));
    } else if (syscall == SYS_write) {
      fprintf(stderr, "write(%d) = ", get_syscall_arg(&regs, 0));
    } else if (syscall < sizeof(syscalls)/sizeof(const char *)) {
      fprintf(stderr, "%s() = ", syscalls[syscall]);
    } else {
      fprintf(stderr, "syscall(%d) = ", syscall);
    }

    if (wait_for_syscall(child) != 0) break;

    if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
      fprintf(stderr, "ERROR PTRACING!\n");
      exit(1);
    }
    retval = regs.rax;
    fprintf(stderr, "%d\n", retval);
  }
}

int wait_for_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}
