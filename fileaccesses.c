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

void do_trace();

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
    waitpid(-1, 0, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0,
           PTRACE_O_TRACESYSGOOD |
           PTRACE_O_TRACEFORK |
           // PTRACE_O_TRACEVFORK | // causes trouble...
           PTRACE_O_TRACEVFORKDONE |
           PTRACE_O_TRACECLONE |
           PTRACE_O_TRACEEXEC);
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    do_trace();
  }
  return 0;
}

int wait_for_syscall_from(pid_t child);
pid_t wait_for_syscall();

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

char *read_string(pid_t child, unsigned long addr) {
    char *val = malloc(4096);
    int allocated = 4096;
    int read = 0;
    unsigned long tmp;
    while (1) {
        if (read + sizeof tmp > allocated) {
            allocated *= 2;
            val = realloc(val, allocated);
        }
        tmp = ptrace(PTRACE_PEEKDATA, child, addr + read);
        if(errno != 0) {
            val[read] = 0;
            break;
        }
        memcpy(val + read, &tmp, sizeof tmp);
        if (memchr(&tmp, 0, sizeof tmp) != NULL)
            break;
        read += sizeof tmp;
    }
    return val;
}

int identify_fd(char *path_buffer, pid_t child, int fd) {
  int ret;
  char *proc = (char *)malloc(PATH_MAX);
  sprintf(proc, "/proc/%d/fd/%d", child, fd);
  ret = readlink(proc, path_buffer, PATH_MAX);
  if (ret == -1) *path_buffer = 0;
  else path_buffer[ret] = 0;
  return ret;
}

static int num_programs = 1;

void do_trace() {
  int status, syscall, retval;
  char *filename = (char *)malloc(PATH_MAX);
  while(1) {
    struct user_regs_struct regs;
    pid_t child = wait_for_syscall();
    if (child == 0) {
      fprintf(stderr, "unknown child exited...\n");
      if (--num_programs == 0) break;
      else continue;
    }

    if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
      fprintf(stderr, "ERROR PTRACING!\n");
      error(1, errno, "error getting registers for %d...", child);
      exit(1);
    }
    syscall = regs.orig_rax;

    if (fd_argument[syscall] >= 0) {
      int fd = get_syscall_arg(&regs, fd_argument[syscall]);
      identify_fd(filename, child, fd);
      fprintf(stderr, "%d: %s(%d == %s) = ",
              child,
              syscalls[syscall],
              fd,
              filename);
    } else if (string_argument[syscall] >= 0) {
      char *arg = read_string(child, get_syscall_arg(&regs, 0));
      fprintf(stderr, "%d: %s(\"%s\") = ", child, syscalls[syscall], arg);
      free(arg);
    } else if (syscall == SYS_exit || syscall == SYS_exit_group) {
      fprintf(stderr, "%d: %s()\n", child, syscalls[syscall]);
    } else if (syscall < sizeof(syscalls)/sizeof(const char *)) {
      fprintf(stderr, "%d: %s() = ", child, syscalls[syscall]);
    } else {
      fprintf(stderr, "%d: syscall(%d) = ", child, syscall);
    }

    if (wait_for_syscall_from(child) != 0) {
      if (--num_programs == 0) break;
      else continue;
    }

    if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
      warn("ERROR PTRACING in %d", child);
    }
    retval = regs.rax;
    if (fd_return[syscall]) {
      identify_fd(filename, child, retval);
      fprintf(stderr, "%d == %s\n", retval, filename);
    } else {
      fprintf(stderr, "%d\n", retval);
    }
    ptrace(PTRACE_SYSCALL, child, 0, 0);
  }
}

int wait_for_syscall_from(pid_t child) {
  int status;
  while (1) {
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    //fprintf(stderr, "\npid:  %d   <-  %d\n", pid, status);
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
      fprintf(stderr, "\ncloned!!!\n");
      num_programs++;
    }
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      fprintf(stderr, "\nforked %d from %d!!!\n", newpid, child);
      waitpid(newpid, 0, 0);
      ptrace(PTRACE_SETOPTIONS, newpid, 0,
             PTRACE_O_TRACESYSGOOD |
             PTRACE_O_TRACEFORK |
             PTRACE_O_TRACEVFORKDONE |
             PTRACE_O_TRACECLONE |
             PTRACE_O_TRACEEXEC);
      num_programs++;
      ptrace(PTRACE_SYSCALL, newpid, 0, 0);
    }
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      //fprintf(stderr, "\nexeced!!! %d from %d\n", newpid, child);
    }
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      fprintf(stderr, "\nvforked %d from %d!!!\n", newpid, child);
    }
    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      //fprintf(stderr, "\nvforked %d from %d!!!\n", newpid, child);
      waitpid(newpid, 0, 0);
      ptrace(PTRACE_SETOPTIONS, newpid, 0,
             PTRACE_O_TRACESYSGOOD |
             PTRACE_O_TRACEFORK |
             PTRACE_O_TRACEVFORKDONE |
             PTRACE_O_TRACECLONE |
             PTRACE_O_TRACEEXEC);
      num_programs++;
      ptrace(PTRACE_SYSCALL, newpid, 0, 0);
    }
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
      return 0;
    if (WIFEXITED(status))
      return 1;
  }
}

pid_t wait_for_syscall() {
  int status;
  while (1) {
    int pid = waitpid(-1, &status, 0);
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
      return pid;
    if (WIFEXITED(status)) {
      return 0;
    }
    ptrace(PTRACE_SYSCALL, pid, 0, 0);
  }
}
