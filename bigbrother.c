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
#include "bigbrother.h"

static const void *my_ptrace_options =
  (void *)(PTRACE_O_TRACESYSGOOD |
           PTRACE_O_TRACEFORK |
           PTRACE_O_TRACEVFORK |
           PTRACE_O_TRACEVFORKDONE |
           PTRACE_O_TRACECLONE |
           PTRACE_O_TRACEEXEC);

static int interesting_path(const char *path) {
  if (memcmp(path, "/dev/", 5) == 0) return 0;
  if (strlen(path) == 0) return 0; /* ?! */
  if (memcmp(path, "/tmp/", 5) == 0) return 0;
  return 1;
}

static void ptrace_syscall(pid_t pid) {
  //fprintf(stderr, "\tcalling ptrace_syscall on %d\n", pid);
  int ret = ptrace(PTRACE_SYSCALL, pid, 0, 0);
  if (ret == -1) {
    error(1, errno, "error calling ptrace_syscall on %d\n", pid);
    exit(1);
  }
}

static long get_syscall_arg(const struct user_regs_struct *regs, int which) {
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

static char *read_a_string(pid_t child, unsigned long addr) {
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

static int identify_fd(char *path_buffer, pid_t child, int fd) {
  int ret;
  char *proc = (char *)malloc(PATH_MAX);
  sprintf(proc, "/proc/%d/fd/%d", child, fd);
  ret = readlink(proc, path_buffer, PATH_MAX);
  free(proc);
  if (ret == -1) *path_buffer = 0;
  else path_buffer[ret] = 0;
  return ret;
}

static int absolute_path(char *path_buffer, pid_t child, const char *path) {
  char *filename = malloc(PATH_MAX);
  sprintf(filename, "/proc/%d/cwd", child);
  chdir(filename);
  realpath(path, path_buffer);
  chdir("/");
  free(filename);
  return 0;
}

static char *read_a_path(pid_t child, unsigned long addr) {
  char *foo = read_a_string(child, addr);
  char *abspath = malloc(PATH_MAX);
  absolute_path(abspath, child, foo);
  free(foo);
  return abspath;
}

static int save_syscall_access(pid_t child,
                               listset **read_from_files,
                               listset **written_to_files,
                               listset **deleted_files) {
  struct user_regs_struct regs;
  int syscall;

  if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
    fprintf(stderr, "ERROR PTRACING %d!\n", child);
    error(1, errno, "error getting registers for %d...", child);
    exit(1);
  }
  syscall = regs.orig_rax;

  if (write_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, write_fd[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        fprintf(stderr, "W: %s(%s)\n", syscalls[syscall], filename);
        insert_to_listset(written_to_files, filename);
        delete_from_listset(deleted_files, filename);
      } else {
        fprintf(stderr, "W~ %s(%s)\n", syscalls[syscall], filename);
        free(filename);
      }
    }
  }
  if (read_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, read_fd[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        fprintf(stderr, "R: %s(%s)\n", syscalls[syscall], filename);
        insert_to_listset(read_from_files, filename);
      } else {
        fprintf(stderr, "R~ %s(%s)\n", syscalls[syscall], filename);
        free(filename);
      }
    }
  }
  if (read_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, read_string[syscall]));
    if (interesting_path(arg)) {
      fprintf(stderr, "R: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(read_from_files, arg);
    } else {
      fprintf(stderr, "R~ %s(%s)\n", syscalls[syscall], arg);
      free(arg);
    }
  }
  if (write_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, write_string[syscall]));
    if (interesting_path(arg)) {
      fprintf(stderr, "W: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(written_to_files, arg);
      delete_from_listset(deleted_files, arg);
    } else {
      fprintf(stderr, "W~ %s(%s)\n", syscalls[syscall], arg);
      free(arg);
    }
  }
  if (unlink_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, unlink_string[syscall]));
    if (interesting_path(arg)) {
      fprintf(stderr, "D: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(deleted_files, arg);
      delete_from_listset(written_to_files, arg);
      delete_from_listset(read_from_files, arg);
    } else {
      fprintf(stderr, "D~ %s(%s)\n", syscalls[syscall], arg);
      free(arg);
    }
  }
  return syscall;
}


int bigbrother_process(char **args,
                       listset **read_from_files,
                       listset **written_to_files,
                       listset **deleted_files) {
  pid_t child = fork();
  if (child == 0) {
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
  } else {
    int num_programs = 1;
    waitpid(-1, 0, 0);
    ptrace(PTRACE_SETOPTIONS, child, 0, my_ptrace_options);
    ptrace_syscall(child); // run until a sycall is attempted

    while (num_programs > 0) {
      struct user_regs_struct regs;

      pid_t child = 0;
      int status, syscall;
    look_for_syscall:
      while (1) {
        //fprintf(stderr, "waiting for any syscall...\n");
        child = waitpid(-1, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
          break;
        } else if (WIFEXITED(status)) {
          //fprintf(stderr, "got an exit from %d...\n", child);
          if (--num_programs <= 0) return 0;
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          fprintf(stderr, "foo execed!!! %d from %d\n", newpid, child);
          exit(1);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
          //fprintf(stderr, "foo vfork done!!! %d\n", child);
          ptrace_syscall(child); // keep going!
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
          fprintf(stderr, "foo cloned!!! %d\n", child);
          exit(1);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
          fprintf(stderr, "foo vforked!!! %d\n", child);
          exit(1);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
          fprintf(stderr, "foo forked!!! %d\n", child);
          exit(1);
        } else if (WIFSIGNALED(status)) {
          fprintf(stderr, "foo signaled!!! %d\n", child);
          exit(1);
        } else if (WIFCONTINUED(status)) {
          fprintf(stderr, "foo continued!!! %d\n", child);
          exit(1);
        } else {
          /* fprintf(stderr, "I do not understand on child %d this %x also %x compare %x... :(\n",
                  child, status, status >> 8, SIGTRAP);
                  exit(1); */
          ptrace_syscall(child);
        }
      }

      syscall = save_syscall_access(child, read_from_files, written_to_files, deleted_files);

      if (is_wait_or_exit[syscall]) {
        /* These syscalls may wait on a child process, so we cannot
           wait for their return, since this may not happen if stop
           processing the child processes.  So my simple
           (non-threaded) approach is just to ignore their return
           value. */
        ptrace(PTRACE_SYSCALL, child, 0, 0); // ignore return value
        goto look_for_syscall;
      }

      ptrace_syscall(child); // run the syscall to its finish

      while (1) {
        //fprintf(stderr, "waiting for syscall from %d...\n", child);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
          break;
        } else if (WIFEXITED(status)) {
          //fprintf(stderr, "we got an exit from %d...\n", child);
          if (--num_programs <= 0) return 0;
          goto look_for_syscall; // no point looking any longer!
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //fprintf(stderr, "\nexeced!!! %d from %d\n", newpid, child);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
          fprintf(stderr, "vfork is done in %d\n", child);
          ptrace_syscall(child); // skip over return value of vfork
          waitpid(child, &status, 0);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //fprintf(stderr, "\nforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, 0);
          //fprintf(stderr, "waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            fprintf(stderr, "error ptracing setoptions n %d\n", newpid);
          }
          //fprintf(stderr, "ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          fprintf(stderr, "\ncloned %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, 0);
          //fprintf(stderr, "waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            fprintf(stderr, "error ptracing setoptions n %d\n", newpid);
          }
          //fprintf(stderr, "ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //fprintf(stderr, "\nvforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, 0);
          //fprintf(stderr, "waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            fprintf(stderr, "error ptracing setoptions n %d\n", newpid);
          }
          //fprintf(stderr, "ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(child);
          ptrace_syscall(newpid);
          goto look_for_syscall;
        } else {
          fprintf(stderr, "I do not understand this event %d\n\n", status);
          exit(1);
        }
        ptrace_syscall(child); // we don't understand id, so keep trying
      }
      ptrace_syscall(child);

    }
  }
  return 0;
}
