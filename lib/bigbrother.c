#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1

#include "bigbrother.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void error(int retval, int errno, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "error: ");
  vfprintf(stderr, format, args);
  if (errno) fprintf(stderr, "\n  %s\n", strerror(errno));
  else fprintf(stderr, "\n");
  va_end(args);
  exit(retval);
}

#ifdef __linux__

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <linux/limits.h>
#include <errno.h>

#include <stdint.h>

#include "syscalls.h"

static const int debug_output = 0;

static inline void debugprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (debug_output) vfprintf(stderr, format, args);
  va_end(args);
}

static const void *my_ptrace_options =
  (void *)(PTRACE_O_TRACESYSGOOD |
           PTRACE_O_TRACEFORK |
           PTRACE_O_TRACEVFORK |
           PTRACE_O_TRACEVFORKDONE |
           PTRACE_O_TRACECLONE |
           PTRACE_O_TRACEEXEC);

static int interesting_path(const char *path) {
  if (strlen(path) == 0) return 0; /* ?! */
  if (path[0] != '/') return 0;
  if (strlen(path) > 4) {
    if (memcmp(path, "/dev/", 5) == 0) return 0;
    if (memcmp(path, "/sys/", 5) == 0) return 0;
    /* the following seems like a good idea, but causes trouble when
       the entire source tree happens to reside in /tmp.  */
    //if (memcmp(path, "/tmp/", 5) == 0) return 0;
    if (memcmp(path, "/proc/", 6) == 0) return 0;
  }
  return 1;
}

#ifdef __x86_64__
struct i386_user_regs_struct {
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  uint32_t esi;
  uint32_t edi;
  uint32_t ebp;
  uint32_t eax;
  uint32_t xds;
  uint32_t xes;
  uint32_t xfs;
  uint32_t xgs;
  uint32_t orig_eax;
  uint32_t eip;
  uint32_t xcs;
  uint32_t eflags;
  uint32_t esp;
  uint32_t xss;
};

static long get_syscall_arg_64(const struct user_regs_struct *regs, int which) {
    switch (which) {
    case 0: return regs->rdi;
    case 1: return regs->rsi;
    case 2: return regs->rdx;
    case 3: return regs->r10;
    case 4: return regs->r8;
    case 5: return regs->r9;
    default: return -1L;
    }
}

static long get_syscall_arg_32(const struct i386_user_regs_struct *regs, int which) {
#else
static long get_syscall_arg_32(const struct user_regs_struct *regs, int which) {
#endif
    switch (which) {
    case 0: return regs->ebx;
    case 1: return regs->ecx;
    case 2: return regs->edx;
    case 3: return regs->esi;
    case 4: return regs->edi;
    case 5: return regs->ebp;
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
  if (snprintf(proc, PATH_MAX, "/proc/%d/fd/%d", child, fd) >= PATH_MAX) {
    fprintf(stderr, "filename too large!!!\n");
    exit(1);
  }
  ret = readlink(proc, path_buffer, PATH_MAX);
  free(proc);
  if (ret == -1) *path_buffer = 0;
  else path_buffer[ret] = 0;
  return ret;
}

static int absolute_path(char *path_buffer, pid_t child, const char *path) {
  char *filename = malloc(PATH_MAX);
  if (snprintf(filename, PATH_MAX, "/proc/%d/cwd", child) >= PATH_MAX) {
    fprintf(stderr, "filename too large!!!\n");
    exit(1);
  }
  char *dirname = getcwd(0, 0);
  chdir(filename);
  if (!realpath(path, path_buffer)) {
    *path_buffer = 0; /* in case of trouble, make it an empty string */
  }
  chdir(dirname);
  free(dirname);
  free(filename);
  return 0;
}

static int absolute_path_at(char *path_buffer, pid_t child, int dirfd, const char *path) {
  char *filename = malloc(PATH_MAX);
  identify_fd(filename, child, dirfd);
  char *dirname = getcwd(0, 0);
  chdir(filename);
  realpath(path, path_buffer);
  chdir(dirname);
  free(dirname);
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

static char *read_a_path_at(pid_t child, int dirfd, unsigned long addr) {
  char *foo = read_a_string(child, addr);
  char *abspath = malloc(PATH_MAX);
  absolute_path_at(abspath, child, dirfd, foo);
  free(foo);
  return abspath;
}

pid_t wait_for_syscall(int firstborn) {
  pid_t child = 0;
  int status = 0;
  while (1) {
    long signal_to_send_back = 0;
    child = waitpid(-firstborn, &status, __WALL);
    if (child == -1) error(1, errno, "trouble waiting");
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
      return child;
    } else if (WIFEXITED(status)) {
      debugprintf("process %d exited!\n", child);
      if (child == firstborn) return -WEXITSTATUS(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSIGNALED(status)) {
      debugprintf("process %d died of a signal!\n", child);
      if (child == firstborn) return -WTERMSIG(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSTOPPED(status)) {
      // ensure that the signal we interrupted is actually delivered.
      switch (WSTOPSIG(status)) {
      case SIGCHLD: // I don't know why forwarding SIGCHLD along causes trouble.  :(
      case SIGTRAP: // SIGTRAP is what we get from ptrace
      case SIGVTALRM: // for some reason this causes trouble with ghc
        debugprintf("ignoring signal %d\n", WSTOPSIG(status));
        break;
      default:
        signal_to_send_back = WSTOPSIG(status);
        debugprintf("sending signal %d\n", signal_to_send_back);
      }
    } else {
      debugprintf("unexpected something from process %d\n", child);
    }
    // tell the child to keep going!
    if (ptrace(PTRACE_SYSCALL, child, 0, (void *)signal_to_send_back) == -1) {
      /* Assume child died and that we will get a WIFEXITED
         shortly. */
    }
  }
}

static int save_syscall_access(pid_t child,
                               hashset *read_from_directories,
                               hashset *read_from_files,
                               hashset *written_to_files,
                               hashset *deleted_files) {
  struct user_regs_struct regs;
  int syscall;

  if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
    debugprintf("error getting registers for %d!\n", child);
    return -1;
  }
#ifdef __x86_64__
  struct i386_user_regs_struct i386_regs;
  if (regs.cs == 0x23) {
    i386_regs.ebx = regs.rbx;
    i386_regs.ecx = regs.rcx;
    i386_regs.edx = regs.rdx;
    i386_regs.esi = regs.rsi;
    i386_regs.edi = regs.rdi;
    i386_regs.ebp = regs.rbp;
    i386_regs.eax = regs.rax;
    i386_regs.orig_eax = regs.orig_rax;
    i386_regs.eip = regs.rip;
    i386_regs.esp = regs.rsp;

    struct i386_user_regs_struct regs = i386_regs;
#endif

    syscall = regs.orig_eax;
    debugprintf("%s() 32 = %d\n", syscalls_32[syscall], syscall);

    if (write_fd_32[syscall] >= 0) {
      int fd = get_syscall_arg_32(&regs, write_fd_32[syscall]);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename)) {
          debugprintf("W: %s(%s)\n", syscalls_32[syscall], filename);
          insert_to_hashset(written_to_files, filename);
          delete_from_hashset(read_from_files, filename);
          delete_from_hashset(deleted_files, filename);
        } else {
          debugprintf("W~ %s(%s)\n", syscalls_32[syscall], filename);
        }
        free(filename);
      }
    }
    if (read_fd_32[syscall] >= 0) {
      int fd = get_syscall_arg_32(&regs, read_fd_32[syscall]);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename) &&
            !is_in_hashset(written_to_files, filename)) {
          debugprintf("R: %s(%s)\n", syscalls_32[syscall], filename);
          insert_to_hashset(read_from_files, filename);
        } else {
          debugprintf("R~ %s(%s)\n", syscalls_32[syscall], filename);
        }
        free(filename);
      }
    }
    if (readdir_fd_32[syscall] >= 0) {
      int fd = get_syscall_arg_32(&regs, readdir_fd_32[syscall]);
      debugprintf("!!!!!!!!! Got readdir with fd %d\n", fd);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename)) {
          debugprintf("readdir: %s(%s)\n", syscalls_32[syscall], filename);
          insert_to_hashset(read_from_directories, filename);
        } else {
          debugprintf("readdir~ %s(%s)\n", syscalls_32[syscall], filename);
        }
        free(filename);
      }
    }
    if (read_string_32[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_32(&regs, read_string_32[syscall]));
      if (interesting_path(arg) && !access(arg, R_OK) &&
          !is_in_hashset(written_to_files, arg)) {
        debugprintf("R: %s(%s)\n", syscalls_32[syscall], arg);
        insert_to_hashset(read_from_files, arg);
      } else {
        debugprintf("R~ %s(%s)\n", syscalls_32[syscall], arg);
      }
      free(arg);
    }
    if (write_string_32[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_32(&regs, write_string_32[syscall]));
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("W: %s(%s)\n", syscalls_32[syscall], arg);
        insert_to_hashset(written_to_files, arg);
        delete_from_hashset(deleted_files, arg);
        delete_from_hashset(read_from_files, arg);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls_32[syscall], arg);
      }
      free(arg);
    }
    if (unlink_string_32[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_32(&regs, unlink_string_32[syscall]));
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("D: %s(%s)\n", syscalls_32[syscall], arg);
        insert_to_hashset(deleted_files, arg);
        delete_from_hashset(written_to_files, arg);
        delete_from_hashset(read_from_files, arg);
        delete_from_hashset(read_from_directories, arg);
      } else {
        debugprintf("D~ %s(%s)\n", syscalls_32[syscall], arg);
      }
      free(arg);
    }
    if (unlinkat_string_32[syscall] >= 0) {
      char *arg = read_a_path_at(child,
                                 get_syscall_arg_32(&regs, 0) /* dirfd */,
                                 get_syscall_arg_32(&regs, 1) /* path */);
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("D: %s(%s)\n", syscalls_32[syscall], arg);
        insert_to_hashset(deleted_files, arg);
        delete_from_hashset(written_to_files, arg);
        delete_from_hashset(read_from_files, arg);
        delete_from_hashset(read_from_directories, arg);
      } else {
        debugprintf("D~ %s(%s)\n", syscalls_32[syscall], arg);
      }
      free(arg);
    }
    if (renameat_string_32[syscall] >= 0) {
      char *arg = read_a_path_at(child,
                                 get_syscall_arg_32(&regs, 2) /* dirfd */,
                                 get_syscall_arg_32(&regs, 3) /* path */);
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("W: %s(%s)\n", syscalls_32[syscall], arg);
        insert_to_hashset(written_to_files, arg);
        delete_from_hashset(deleted_files, arg);
        delete_from_hashset(read_from_files, arg);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls_32[syscall], arg);
      }
      free(arg);
    }
#ifdef __x86_64__
  } else {
    syscall = regs.orig_rax;
    if (syscall < 0) {
      debugprintf("invalid system call %d\n", syscall);
      return -1;
    }
    debugprintf("%s() 64\n", syscalls_64[syscall]);

    if (write_fd_64[syscall] >= 0) {
      int fd = get_syscall_arg_64(&regs, write_fd_64[syscall]);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename)) {
          debugprintf("W: %s(%s)\n", syscalls_64[syscall], filename);
          insert_to_hashset(written_to_files, filename);
          delete_from_hashset(read_from_files, filename);
          delete_from_hashset(deleted_files, filename);
        } else {
          debugprintf("W~ %s(%s)\n", syscalls_64[syscall], filename);
        }
        free(filename);
      }
    }
    if (read_fd_64[syscall] >= 0) {
      int fd = get_syscall_arg_64(&regs, read_fd_64[syscall]);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename) &&
            !is_in_hashset(written_to_files, filename)) {
          debugprintf("R: %s(%s)\n", syscalls_64[syscall], filename);
          insert_to_hashset(read_from_files, filename);
        } else {
          debugprintf("R~ %s(%s)\n", syscalls_64[syscall], filename);
        }
        free(filename);
      }
    }
    if (readdir_fd_64[syscall] >= 0) {
      int fd = get_syscall_arg_64(&regs, readdir_fd_64[syscall]);
      debugprintf("!!!!!!!!! Got readdir with fd %d\n", fd);
      if (fd >= 0) {
        char *filename = (char *)malloc(PATH_MAX);
        identify_fd(filename, child, fd);
        if (interesting_path(filename)) {
          debugprintf("readdir: %s(%s)\n", syscalls_64[syscall], filename);
          insert_to_hashset(read_from_directories, filename);
        } else {
          debugprintf("readdir~ %s(%s)\n", syscalls_64[syscall], filename);
        }
        free(filename);
      }
    }
    if (read_string_64[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_64(&regs, read_string_64[syscall]));
      if (interesting_path(arg) && !access(arg, R_OK) &&
          !is_in_hashset(written_to_files, arg)) {
        debugprintf("R: %s(%s)\n", syscalls_64[syscall], arg);
        insert_to_hashset(read_from_files, arg);
      } else {
        debugprintf("R~ %s(%s)\n", syscalls_64[syscall], arg);
      }
      free(arg);
    }
    if (write_string_64[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_64(&regs, write_string_64[syscall]));
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("W: %s(%s)\n", syscalls_64[syscall], arg);
        insert_to_hashset(written_to_files, arg);
        delete_from_hashset(deleted_files, arg);
        delete_from_hashset(read_from_files, arg);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls_64[syscall], arg);
      }
      free(arg);
    }
    if (unlink_string_64[syscall] >= 0) {
      char *arg = read_a_path(child, get_syscall_arg_64(&regs, unlink_string_64[syscall]));
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("D: %s(%s)\n", syscalls_64[syscall], arg);
        insert_to_hashset(deleted_files, arg);
        delete_from_hashset(written_to_files, arg);
        delete_from_hashset(read_from_files, arg);
        delete_from_hashset(read_from_directories, arg);
      } else {
        debugprintf("D~ %s(%s)\n", syscalls_64[syscall], arg);
      }
      free(arg);
    }
    if (unlinkat_string_64[syscall] >= 0) {
      char *arg = read_a_path_at(child,
                                 get_syscall_arg_64(&regs, 0) /* dirfd */,
                                 get_syscall_arg_64(&regs, 1) /* path */);
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("D: %s(%s)\n", syscalls_64[syscall], arg);
        insert_to_hashset(deleted_files, arg);
        delete_from_hashset(written_to_files, arg);
        delete_from_hashset(read_from_files, arg);
        delete_from_hashset(read_from_directories, arg);
      } else {
        debugprintf("D~ %s(%s)\n", syscalls_64[syscall], arg);
      }
      free(arg);
    }
    if (renameat_string_64[syscall] >= 0) {
      char *arg = read_a_path_at(child,
                                 get_syscall_arg_64(&regs, 2) /* dirfd */,
                                 get_syscall_arg_64(&regs, 3) /* path */);
      if (interesting_path(arg) && !access(arg, W_OK)) {
        debugprintf("W: %s(%s)\n", syscalls_64[syscall], arg);
        insert_to_hashset(written_to_files, arg);
        delete_from_hashset(deleted_files, arg);
        delete_from_hashset(read_from_files, arg);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls_64[syscall], arg);
      }
      free(arg);
    }
  }
#endif
  return syscall;
}


int bigbrother_process(const char *workingdir,
                       pid_t *child_ptr,
                       int stdouterrfd,
                       char **args,
                       hashset *read_from_directories,
                       hashset *read_from_files,
                       hashset *written_to_files,
                       hashset *deleted_files) {
  initialize_hashset(read_from_directories);
  initialize_hashset(read_from_files);
  initialize_hashset(written_to_files);
  initialize_hashset(deleted_files);

  pid_t firstborn = fork();
  if (firstborn == -1) {
  }
  setpgid(firstborn, firstborn); // causes grandchildren to be killed along with firstborn

  if (firstborn == 0) {
    if (stdouterrfd > 0) {
      close(1);
      close(2);
      dup(stdouterrfd);
      dup(stdouterrfd);
    }
    if (workingdir && chdir(workingdir) != 0) return -1;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
  } else {
    *child_ptr = firstborn;
    waitpid(firstborn, 0, __WALL);
    ptrace(PTRACE_SETOPTIONS, firstborn, 0, my_ptrace_options);
    if (ptrace(PTRACE_SYSCALL, firstborn, 0, 0) == -1) {
      return -1;
    }

    while (1) {
      pid_t child = wait_for_syscall(firstborn);
      if (child <= 0) {
        debugprintf("Returning with exit value %d\n", -child);
        return -child;
      }

      if (save_syscall_access(child, read_from_directories,
                              read_from_files, written_to_files,
                              deleted_files) == -1) {
        /* We were unable to read the process's registers.  Assume
           that this is bad news, and that we should exit.  I'm not
           sure what else to do here. */
        return -1;
      }

      ptrace(PTRACE_SYSCALL, child, 0, 0); // ignore return value
    }
  }
  return 0;
}

#else

/* freebsd */

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <errno.h>
#include <machine/reg.h>
#include <signal.h>

#include <stdint.h>

#include "syscalls.h"

static const int debug_output = 1;

static inline void debugprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (debug_output) vfprintf(stderr, format, args);
  va_end(args);
}

static int interesting_path(const char *path) {
  if (strlen(path) == 0) return 0; /* ?! */
  if (path[0] != '/') return 0;
  if (strlen(path) > 4) {
    if (memcmp(path, "/dev/", 5) == 0) return 0;
    if (memcmp(path, "/sys/", 5) == 0) return 0;
    /* the following seems like a good idea, but causes trouble when
       the entire source tree happens to reside in /tmp.  */
    //if (memcmp(path, "/tmp/", 5) == 0) return 0;
    if (memcmp(path, "/proc/", 6) == 0) return 0;
  }
  return 1;
}

static long get_syscall_arg(const struct reg *regs, int which) {
  switch (which) {
  case 0:
    return regs->r_rdi;
  case 1:
    return regs->r_rsi;
  case 2:
    return regs->r_rdx;
  case 3:
    return regs->r_rcx;
  case 4:
    return regs->r_r8;
  case 5:
    return regs->r_r9;
  }
  return 0;
}

static char *read_a_string(pid_t child, unsigned long addr) {
    char *val = malloc(4096);
    int allocated = 4096;
    struct ptrace_io_desc iodesc;
    iodesc.piod_op = PIOD_READ_D;
    iodesc.piod_offs = (void *)addr;
    iodesc.piod_addr = val;
    iodesc.piod_len = allocated;
    ptrace(PT_IO, child, (char *)&iodesc, 0);
    return val;
}

static int identify_fd(char *path_buffer, pid_t child, int fd) {
  int ret;
  char *proc = (char *)malloc(PATH_MAX);
  if (snprintf(proc, PATH_MAX, "/proc/%d/fd/%d", child, fd) >= PATH_MAX) {
    fprintf(stderr, "filename too large!!!\n");
    exit(1);
  }
  ret = readlink(proc, path_buffer, PATH_MAX);
  free(proc);
  if (ret == -1) *path_buffer = 0;
  else path_buffer[ret] = 0;
  return ret;
}

static int absolute_path(char *path_buffer, pid_t child, const char *path) {
  char *filename = malloc(PATH_MAX);
  if (snprintf(filename, PATH_MAX, "/proc/%d/cwd", child) >= PATH_MAX) {
    fprintf(stderr, "filename too large!!!\n");
    exit(1);
  }
  char *dirname = getcwd(0, 0);
  chdir(filename);
  if (!realpath(path, path_buffer)) {
    *path_buffer = 0; /* in case of trouble, make it an empty string */
  }
  chdir(dirname);
  free(dirname);
  free(filename);
  return 0;
}

static int absolute_path_at(char *path_buffer, pid_t child, int dirfd, const char *path) {
  char *filename = malloc(PATH_MAX);
  identify_fd(filename, child, dirfd);
  char *dirname = getcwd(0, 0);
  chdir(filename);
  realpath(path, path_buffer);
  chdir(dirname);
  free(dirname);
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

static char *read_a_path_at(pid_t child, int dirfd, unsigned long addr) {
  char *foo = read_a_string(child, addr);
  char *abspath = malloc(PATH_MAX);
  absolute_path_at(abspath, child, dirfd, foo);
  free(foo);
  return abspath;
}

pid_t wait_for_syscall(int firstborn) {
  pid_t child = 0;
  int status = 0;
  while (1) {
    long signal_to_send_back = 0;
    child = waitpid(-firstborn, &status, 0);
    debugprintf("just waited and got child %d with status %d\n",
		child, status);
    if (child == -1) error(1, errno, "trouble waiting");
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
      debugprintf("we saw a syscall of some sort!\n");
      return child;
    } else if (WIFEXITED(status)) {
      debugprintf("process %d exited!\n", child);
      if (child == firstborn) return -WEXITSTATUS(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSIGNALED(status)) {
      debugprintf("process %d died of a signal!\n", child);
      if (child == firstborn) return -WTERMSIG(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSTOPPED(status)) {
      // ensure that the signal we interrupted is actually delivered.
      switch (WSTOPSIG(status)) {
      case SIGCHLD: // I don't know why forwarding SIGCHLD along causes trouble.  :(
      case SIGTRAP: // SIGTRAP is what we get from ptrace
      case SIGVTALRM: // for some reason this causes trouble with ghc
        debugprintf("ignoring signal %d\n", WSTOPSIG(status));
        break;
      default:
        signal_to_send_back = WSTOPSIG(status);
        debugprintf("sending signal %d\n", signal_to_send_back);
      }
    } else {
      debugprintf("unexpected something from process %d\n", child);
    }
    // tell the child to keep going!
    if (ptrace(PT_TO_SCE, child, (caddr_t)1, signal_to_send_back) == -1) {
      /* Assume child died and that we will get a WIFEXITED
         shortly. */
    }
  }
}

static int save_syscall_access(pid_t child,
                               hashset *read_from_directories,
                               hashset *read_from_files,
                               hashset *written_to_files,
                               hashset *deleted_files) {
  printf("starting save_syscall_access...\n");
  struct reg regs;
  int syscall;

  if (ptrace(PT_GETREGS, child, (char *)&regs, 0) == -1) {
    debugprintf("error getting registers for %d!\n", child);
    return -1;
  }

  syscall = regs.r_rax;
  debugprintf("%s() 32 = %d\n", syscalls_32[syscall], syscall);

  if (write_fd_32[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, write_fd_32[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        debugprintf("W: %s(%s)\n", syscalls_32[syscall], filename);
        insert_to_hashset(written_to_files, filename);
        delete_from_hashset(read_from_files, filename);
        delete_from_hashset(deleted_files, filename);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls_32[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_fd_32[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, read_fd_32[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename) &&
          !is_in_hashset(written_to_files, filename)) {
        debugprintf("R: %s(%s)\n", syscalls_32[syscall], filename);
        insert_to_hashset(read_from_files, filename);
      } else {
        debugprintf("R~ %s(%s)\n", syscalls_32[syscall], filename);
      }
      free(filename);
    }
  }
  if (readdir_fd_32[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, readdir_fd_32[syscall]);
    debugprintf("!!!!!!!!! Got readdir with fd %d\n", fd);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        debugprintf("readdir: %s(%s)\n", syscalls_32[syscall], filename);
        insert_to_hashset(read_from_directories, filename);
      } else {
        debugprintf("readdir~ %s(%s)\n", syscalls_32[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_string_32[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, read_string_32[syscall]));
    if (interesting_path(arg) && !access(arg, R_OK) &&
        !is_in_hashset(written_to_files, arg)) {
      debugprintf("R: %s(%s)\n", syscalls_32[syscall], arg);
      insert_to_hashset(read_from_files, arg);
    } else {
      debugprintf("R~ %s(%s)\n", syscalls_32[syscall], arg);
    }
    free(arg);
  }
  if (write_string_32[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, write_string_32[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls_32[syscall], arg);
      insert_to_hashset(written_to_files, arg);
      delete_from_hashset(deleted_files, arg);
      delete_from_hashset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls_32[syscall], arg);
    }
    free(arg);
  }
  if (unlink_string_32[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, unlink_string_32[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls_32[syscall], arg);
      insert_to_hashset(deleted_files, arg);
      delete_from_hashset(written_to_files, arg);
      delete_from_hashset(read_from_files, arg);
      delete_from_hashset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls_32[syscall], arg);
    }
    free(arg);
  }
  if (unlinkat_string_32[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 0) /* dirfd */,
                               get_syscall_arg(&regs, 1) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls_32[syscall], arg);
      insert_to_hashset(deleted_files, arg);
      delete_from_hashset(written_to_files, arg);
      delete_from_hashset(read_from_files, arg);
      delete_from_hashset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls_32[syscall], arg);
    }
    free(arg);
  }
  if (renameat_string_32[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 2) /* dirfd */,
                               get_syscall_arg(&regs, 3) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls_32[syscall], arg);
      insert_to_hashset(written_to_files, arg);
      delete_from_hashset(deleted_files, arg);
      delete_from_hashset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls_32[syscall], arg);
    }
    free(arg);
  }
  return syscall;
}


int bigbrother_process(const char *workingdir,
                       pid_t *child_ptr,
                       int stdouterrfd,
                       char **args,
                       hashset *read_from_directories,
                       hashset *read_from_files,
                       hashset *written_to_files,
                       hashset *deleted_files) {
  printf("hello bigbrother_process!\n");
  initialize_hashset(read_from_directories);
  initialize_hashset(read_from_files);
  initialize_hashset(written_to_files);
  initialize_hashset(deleted_files);

  debugprintf("About to fork... %s\n", args[0]);
  pid_t firstborn = vfork();
  if (firstborn == 0) {
    ptrace(PT_TRACE_ME, 0, 0, 0);
    execvp(args[0], args);
    error(1, 0, "execvp failed: %s", args[0]);
  } else {
    /* *child_ptr = firstborn; */
    debugprintf("I have now forked...\n");
    if (waitpid(firstborn, 0, 0) < 0) {
      error(1, 0, "unespected bad wait for %d!!!\n", firstborn);
    }
    debugprintf("the process has now stopped!\n");
    
    //ptrace(PTRACE_SETOPTIONS, firstborn, 0, my_ptrace_options);
    if (ptrace(PT_TO_SCE, firstborn, (caddr_t)1, 0) == -1) {
      return -1;
    }
    debugprintf("we have ptraced it a bit\n");

    while (1) {
      debugprintf("about to wait for syscall from firstborn %d\n",
		  firstborn);
      pid_t child = wait_for_syscall(firstborn);
      if (child <= 0) {
        debugprintf("Returning with exit value %d\n", -child);
        return -child;
      }
      debugprintf("wait mentioned child %d\n", child);

      if (save_syscall_access(child, read_from_directories,
                              read_from_files, written_to_files,
                              deleted_files) == -1) {
        /* We were unable to read the process's registers.  Assume
           that this is bad news, and that we should exit.  I'm not
           sure what else to do here. */
        return -1;
      }

      ptrace(PT_TO_SCE, child, (caddr_t)1, 0); // ignore return value
    }
  }
  return 0;
}
 
#endif
