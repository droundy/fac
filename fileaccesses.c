#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/limits.h>
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

int identify_fd(char *path_buffer, int child, int fd) {
  int ret;
  char *proc = (char *)malloc(PATH_MAX);
  sprintf(proc, "/proc/%d/fd/%d", child, fd);
  ret = readlink(proc, path_buffer, PATH_MAX);
  if (ret == -1) *path_buffer = 0;
  else path_buffer[ret] = 0;
  return ret;
}

void do_trace(pid_t child) {
  int status, syscall, retval;
  char *filename = (char *)malloc(PATH_MAX);
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

    if (fd_argument[syscall] >= 0) {
      int fd = get_syscall_arg(&regs, fd_argument[syscall]);
      identify_fd(filename, child, fd);
      fprintf(stderr, "%s(%d == %s) = ",
              syscalls[syscall],
              fd,
              filename);
    } else if (syscall == SYS_mmap) {
      identify_fd(filename, child, get_syscall_arg(&regs, 0));
      fprintf(stderr, "%s(%d == %s) = ",
              syscalls[syscall],
              get_syscall_arg(&regs, 0),
              filename);
    } else if (syscall == SYS_exit || syscall == SYS_exit_group) {
      fprintf(stderr, "%s()\n", syscalls[syscall]);
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
