#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1

#include "bigbrother.h"
#include "posixmodel.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> /* for flags to open(2) */

static inline void error(int retval, int errno, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "error: ");
  vfprintf(stderr, format, args);
  if (errno) fprintf(stderr, "\n  %s\n", strerror(errno));
  va_end(args);
  exit(retval);
}

static const int debug_output = 0;

static inline void debugprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (debug_output) vfprintf(stdout, format, args);
  va_end(args);
}

static inline char *debug_realpath(struct inode *i) {
  // WARNING: only use this function as an input to debugprintf!
  if (debug_output) return model_realpath(i);
  return 0;
}

#ifdef __linux__

#include <sys/stat.h>

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <linux/limits.h>
#include <errno.h>

#include <stdint.h>

#include "linux-syscalls.h"

static const void *my_ptrace_options =
  (void *)(PTRACE_O_TRACESYSGOOD |
           PTRACE_O_TRACEFORK |
           PTRACE_O_TRACEVFORK |
           PTRACE_O_TRACEVFORKDONE |
           PTRACE_O_TRACECLONE |
           PTRACE_O_TRACEEXEC);

enum arguments {
  RETURN_VALUE = -1
};

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
    case RETURN_VALUE: return regs->rax;
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
    case RETURN_VALUE: return regs->eax;
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

static pid_t wait_for_syscall(struct posixmodel *m, int firstborn) {
  pid_t child = 0;
  int status = 0;
  while (1) {
    long signal_to_send_back = 0;
    child = waitpid(-firstborn, &status, __WALL);
    if (child == -1) error(1, errno, "trouble waiting");
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
      return child;
    } else if (WIFEXITED(status)) {
      debugprintf("%d: exited -> %d\n", child, -WEXITSTATUS(status));
      if (child == firstborn) return -WEXITSTATUS(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSIGNALED(status)) {
      debugprintf("process %d died of a signal!\n", child);
      if (child == firstborn) return -WTERMSIG(status);
      continue; /* no need to do anything more for this guy */
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_FORK << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      debugprintf("%ld: forked from %d\n", pid, child);
      model_chdir(m, model_cwd(m, child), ".", pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_VFORK << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      debugprintf("%ld: vforked from %d\n", pid, child);
      model_chdir(m, model_cwd(m, child), ".", pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_CLONE << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      debugprintf("%ld: cloned from %d\n", pid, child);
      model_newthread(m, child, pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_EXEC << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      debugprintf("%ld: execed from %d\n", pid, child);
      model_chdir(m, model_cwd(m, child), ".", pid);
    } else if (WIFSTOPPED(status)) {
      // ensure that the signal we interrupted is actually delivered.
      switch (WSTOPSIG(status)) {
      case SIGCHLD: // I don't know why forwarding SIGCHLD along causes trouble.  :(
      case SIGTRAP: // SIGTRAP is what we get from ptrace
      case SIGVTALRM: // for some reason this causes trouble with ghc
        debugprintf("%d: ignoring signal %d\n", child, WSTOPSIG(status));
        break;
      default:
        signal_to_send_back = WSTOPSIG(status);
        debugprintf("%d: sending signal %d\n", child, signal_to_send_back);
      }
    } else {
      debugprintf("%d: unexpected something\n", child);
    }
    // tell the child to keep going!
    if (ptrace(PTRACE_SYSCALL, child, 0, (void *)signal_to_send_back) == -1) {
      /* Assume child died and that we will get a WIFEXITED
         shortly. */
    }
  }
}

static const char *get_registers(pid_t child, void **voidregs,
                                 long (**get_syscall_arg)(void *regs, int which)) {
  struct user_regs_struct *regs = malloc(sizeof(struct user_regs_struct));

  if (ptrace(PTRACE_GETREGS, child, NULL, regs) == -1) {
    debugprintf("error getting registers for %d!\n", child);
    free(regs);
    return 0;
  }
#ifdef __x86_64__
  if (regs->cs == 0x23) {
    struct i386_user_regs_struct *i386_regs = malloc(sizeof(struct i386_user_regs_struct));
    i386_regs->ebx = regs->rbx;
    i386_regs->ecx = regs->rcx;
    i386_regs->edx = regs->rdx;
    i386_regs->esi = regs->rsi;
    i386_regs->edi = regs->rdi;
    i386_regs->ebp = regs->rbp;
    i386_regs->eax = regs->rax;
    i386_regs->orig_eax = regs->orig_rax;
    i386_regs->eip = regs->rip;
    i386_regs->esp = regs->rsp;
    free(regs);
    *voidregs = i386_regs;
    struct i386_user_regs_struct *regs = i386_regs;
#else
    *voidregs = regs;
#endif
    *get_syscall_arg = (long (*)(void *regs, int which))get_syscall_arg_32;
    if (regs->orig_eax < 0) {
      free(regs);
      return 0;
    }
    return syscalls_32[regs->orig_eax];
#ifdef __x86_64__
  } else {
    *voidregs = regs;
    *get_syscall_arg = (long (*)(void *regs, int which))get_syscall_arg_64;
    if (regs->orig_rax < 0) {
      free(regs);
      return 0;
    }
    return syscalls_64[regs->orig_rax];
  }
#endif
}

static long wait_for_return_value(struct posixmodel *m, pid_t child) {
  ptrace(PTRACE_SYSCALL, child, 0, 0); // ignore return value
  wait_for_syscall(m, -child);
  void *regs = 0;
  long (*get_syscall_arg)(void *regs, int which) = 0;
  get_registers(child, &regs, &get_syscall_arg);
  long retval = get_syscall_arg(regs, RETURN_VALUE);
  free(regs);
  return retval;
}

static int save_syscall_access(pid_t child, struct posixmodel *m) {
  void *regs = 0;
  long (*get_syscall_arg)(void *regs, int which) = 0;

  const char *name = get_registers(child, &regs, &get_syscall_arg);
  if (!name) {
    free(regs);
    return -1;
  }

  //debugprintf("%d: %s(?)\n", child, name);

  /*  TODO:

      unlink, unlinkat symlink symlinkat
      chroot? mkdir? mkdirat? rmdir? rmdirat?
  */

  if (!strcmp(name, "open") || !strcmp(name, "openat")) {
    char *arg;
    struct inode *cwd;
    long flags;
    if (!strcmp(name, "open")) {
      arg = read_a_string(child, get_syscall_arg(regs, 0));
      flags = get_syscall_arg(regs, 1);
      cwd = model_cwd(m, child);
    } else {
      arg = read_a_string(child, get_syscall_arg(regs, 1));
      flags = get_syscall_arg(regs, 2);
      cwd = lookup_fd(m, child, get_syscall_arg(regs, 0));
    }
    int fd = wait_for_return_value(m, child);
    if (flags & O_DIRECTORY) {
      if (!strcmp(name, "open")) {
        debugprintf("%d: opendir('%s') -> %d\n", child, arg, fd);
      } else {
        debugprintf("%d: opendirat(%d, '%s') -> %d\n", child, get_syscall_arg(regs, 0), arg, fd);
      }
      model_opendir(m, cwd, arg, child, fd);
    } else if (flags & (O_WRONLY | O_RDWR)) {
      debugprintf("%d: open('%s', 'w') -> %d\n", child, arg, fd);
      if (fd >= 0) {
        struct inode *i = model_creat(m, cwd, arg);
        if (i) debugprintf("%d: has written %s\n", child, debug_realpath(i));
      }
    } else {
      debugprintf("%d: open('%s', 'r') -> %d\n", child, arg, fd);
      struct inode *i = model_stat(m, cwd, arg);
      if (i && i->type == is_file) {
        debugprintf("%d: has read %s\n", child, debug_realpath(i));
        i->is_read = true;
      } else if (i && i->type == is_directory) {
        model_opendir(m, cwd, arg, child, fd);
      }
    }
    free(arg);
  } else if (!strcmp(name, "creat") || !strcmp(name, "truncate")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int fd = wait_for_return_value(m, child);
    debugprintf("%d: %s('%s') -> %d\n", child, name, arg, fd);
    if (fd >= 0) {
      struct inode *i = model_stat(m, model_cwd(m, child), arg);
      if (i) i->is_written = true;
    }
    free(arg);
  } else if (!strcmp(name, "lstat") || !strcmp(name, "lstat64") ||
             !strcmp(name, "readlink") || !strcmp(name, "readlinkat")) {
    char *arg;
    struct inode *cwd;
    if (strcmp(name, "readlinkat")) {
      arg = read_a_string(child, get_syscall_arg(regs, 0));
      cwd = model_cwd(m, child);
    } else {
      arg = read_a_string(child, get_syscall_arg(regs, 1));
      cwd = lookup_fd(m, child, get_syscall_arg(regs, 0));
    }
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: %s('%s') -> %d\n", child, name, arg, retval);
    struct inode *i = model_lstat(m, cwd, arg);
    if (i && i->type != is_directory) {
      debugprintf("%d: has read %s\n", child, debug_realpath(i));
      i->is_read = true;
    }
    free(arg);
  } else if (!strcmp(name, "stat") || !strcmp(name, "stat64")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: %s('%s') -> %d\n", child, name, arg, retval);
    struct inode *i = model_stat(m, model_cwd(m, child), arg);
    if (i && i->type != is_directory) {
      debugprintf("%d: has read %s\n", child, debug_realpath(i));
      i->is_read = true;
    }
    free(arg);
  } else if (!strcmp(name, "execve") || !strcmp(name, "execveat")) {
    char *arg;
    struct inode *cwd;
    if (!strcmp(name, "execve")) {
      arg = read_a_string(child, get_syscall_arg(regs, 0));
      cwd = model_cwd(m, child);
    } else {
      arg = read_a_string(child, get_syscall_arg(regs, 1));
      cwd = lookup_fd(m, child, get_syscall_arg(regs, 0));
    }
    if (strlen(arg)) {
      debugprintf("%d: %s('%s')\n", child, name, arg);
      struct inode *i = model_stat(m, cwd, arg);
      if (i) {
        debugprintf("%d: has read %s\n", child, debug_realpath(i));
        i->is_read = true;
      }
    }
    free(arg);
  } else if (!strcmp(name, "rename") || !strcmp(name, "renameat")) {
    char *from, *to;
    struct inode *cwd;
    if (!strcmp(name, "rename")) {
      from = read_a_string(child, get_syscall_arg(regs, 0));
      to = read_a_string(child, get_syscall_arg(regs, 1));
      cwd = model_cwd(m, child);
    } else {
      from = read_a_string(child, get_syscall_arg(regs, 1));
      to = read_a_string(child, get_syscall_arg(regs, 2));
      cwd = lookup_fd(m, child, get_syscall_arg(regs, 0));
    }
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: rename('%s', '%s') -> %d\n", child, from, to, retval);
    model_rename(m, cwd, from, to);
    free(from);
    free(to);
  } else if (!strcmp(name, "link") || !strcmp(name, "linkat")) {
    char *from, *to;
    struct inode *cwd;
    if (!strcmp(name, "link")) {
      from = read_a_string(child, get_syscall_arg(regs, 0));
      to = read_a_string(child, get_syscall_arg(regs, 1));
      cwd = model_cwd(m, child);
    } else {
      from = read_a_string(child, get_syscall_arg(regs, 1));
      to = read_a_string(child, get_syscall_arg(regs, 2));
      cwd = lookup_fd(m, child, get_syscall_arg(regs, 0));
    }
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: link('%s', '%s') -> %d\n", child, from, to, retval);
    struct inode *f = model_stat(m, cwd, from);
    if (f) f->is_read = true;
    struct inode *t = model_stat(m, cwd, to);
    if (t) t->is_read = true;
    free(from);
    free(to);
  } else if (!strcmp(name, "close")) {
    int fd = get_syscall_arg(regs, 0);
    debugprintf("%d: close(%d)\n", child, fd);
    model_close(m, child, fd);
    wait_for_return_value(m, child);
  } else if (!strcmp(name, "dup2") || !strcmp(name, "dup3")) {
    int fd1 = get_syscall_arg(regs, 0);
    int fd2 = get_syscall_arg(regs, 1);
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: %s(%d, %d) -> %d\n", child, name, fd1, fd2, retval);
    if (retval == fd2) model_dup2(m, child, fd1, fd2);
  } else if (!strcmp(name, "dup")) {
    int fd1 = get_syscall_arg(regs, 0);
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: dup(%d) -> %d\n", child, fd1, retval);
    if (retval >= 0) model_dup2(m, child, fd1, retval);
  } else if (!strcmp(name, "getdents") || !strcmp(name, "getdents64")) {
    int fd = get_syscall_arg(regs, 0);
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: %s(%d) -> %d\n", child, name, fd, retval);
    struct inode *i = lookup_fd(m, child, fd);
    if (i && i->type == is_directory) {
      i->is_read = true;
      debugprintf("%d: have read '%s'\n", child, debug_realpath(i));
    }
  } else if (!strcmp(name, "chdir")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: chdir(%s) -> %d\n", child, arg, retval);
    model_chdir(m, model_cwd(m, child), arg, child);
    free(arg);
  }

  free(regs);
  return 0;
}


int bigbrother_process(const char *workingdir,
                       pid_t *child_ptr,
                       int stdouterrfd,
                       char **args,
                       hashset *read_from_directories,
                       hashset *read_from_files,
                       hashset *written_to_files,
                       hashset *deleted_files) {
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

    struct posixmodel m;
    init_posixmodel(&m);
    {
      char *cwd = getcwd(0,0);
      model_chdir(&m, 0, cwd, firstborn);
      if (workingdir) model_chdir(&m, model_cwd(&m, firstborn), workingdir, firstborn);
      free(cwd);
    }

    while (1) {
      pid_t child = wait_for_syscall(&m, firstborn);
      if (child <= 0) {
        debugprintf("Returning with exit value %d\n", -child);
        model_output(&m, read_from_directories, read_from_files, written_to_files);
        free_posixmodel(&m);
        return -child;
      }

      if (save_syscall_access(child, &m) == -1) {
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <sys/syscall.h>
#include "freebsd-syscalls.h"
int nsyscalls = sizeof(syscalls_freebsd)/sizeof(syscalls_freebsd[0]);

void read_ktrace(int ktrfd, struct posixmodel *m);

int bigbrother_process(const char *workingdir,
                       pid_t *child_ptr,
                       int stdouterrfd,
                       char **args,
                       hashset *read_from_directories,
                       hashset *read_from_files,
                       hashset *written_to_files,
                       hashset *deleted_files) {
  const char *templ = "/tmp/fac-XXXXXX";
  char *namebuf = malloc(strlen(templ)+1);
  strcpy(namebuf, templ);
  int tracefd = mkstemp(namebuf);
  
  pid_t firstborn = fork();
  if (firstborn == -1) {
  }
  setpgid(firstborn, firstborn); // causes grandchildren to be killed along with firstborn

  if (firstborn == 0) {
    int retval = ktrace(namebuf, KTROP_SET,
                        KTRFAC_SYSCALL | KTRFAC_NAMEI |
                        KTRFAC_SYSRET | KTRFAC_INHERIT,
                        getpid());
    if (retval) error(1, errno, "ktrace gives %d for %s", retval, namebuf);
    unlink(namebuf);

    if (stdouterrfd > 0) {
      close(1);
      close(2);
      dup(stdouterrfd);
      dup(stdouterrfd);
    }
    if (workingdir && chdir(workingdir) != 0) return -1;
    execvp(args[0], args);
  }
  *child_ptr = firstborn;

  /* for debugging purposes, send trace info to stdout */
  /* printf("dumping trace info from %s... %d\n", */
  /*  namebuf, (int)lseek(tracefd, 0, SEEK_END)); */
  free(namebuf);

  int status = 0;
  waitpid(firstborn, &status, 0);

  struct posixmodel m;
  init_posixmodel(&m);
  {
    char *cwd = getcwd(0,0);
    model_chdir(&m, 0, cwd, firstborn);
    if (workingdir) model_chdir(&m, model_cwd(&m, firstborn), workingdir, firstborn);
    free(cwd);
  }

  read_ktrace(tracefd, &m);

  fflush(stdout);
  fflush(stderr);

  model_output(&m, read_from_directories, read_from_files, written_to_files);

  if (WIFEXITED(status)) return -WEXITSTATUS(status);
  return 1;
}

union ktr_stuff {
  struct ktr_syscall sc;
  struct ktr_sysret sr;
  char ni[4096];
};

int read_ktrace_entry(int tracefd, struct ktr_header *kth,
                      union ktr_stuff **buf, int *size) {
  /* We return 0 when we have successfully read an entry. */
  if (read(tracefd, kth, sizeof(struct ktr_header)) != sizeof(struct ktr_header)) {
    return 1;
  }
  if (kth->ktr_len+1 > *size) {
    *buf = realloc(*buf, kth->ktr_len+1);
    *size = kth->ktr_len+1;
  }
  if (read(tracefd, *buf, kth->ktr_len) != kth->ktr_len) {
    printf("error reading.\n");
    return 1;
  }
  if (kth->ktr_type == KTR_NAMEI) (*buf)->ni[kth->ktr_len] = 0;
  return 0;
}

char *read_ktrace_namei(int tracefd, pid_t pid, struct ktr_header *kth,
                        union ktr_stuff **buf, int *size) {
  kth->ktr_pid = -1;
  while (kth->ktr_pid != pid) {
    if (read_ktrace_entry(tracefd, kth, buf, size)) {
      printf("ERROR READING ANMEIGE\n");
      exit(1);
    }
  }
  assert(kth->ktr_pid == pid);
  assert(kth->ktr_type == KTR_NAMEI);
  return strdup((*buf)->ni);
}

int read_ktrace_sysret(int tracefd, pid_t pid, struct ktr_header *kth,
                         union ktr_stuff **buf, int *size) {
  kth->ktr_pid = -1;
  while (kth->ktr_pid != pid) {
    if (read_ktrace_entry(tracefd, kth, buf, size)) {
      printf("ERROR READING ANMEIGE\n");
      exit(1);
    }
  }
  assert(kth->ktr_pid == pid);
  assert(kth->ktr_type == KTR_SYSRET);
  return (*buf)->sr.ktr_retval;
}

void read_ktrace(int tracefd, struct posixmodel *m) {
  lseek(tracefd, 0, SEEK_SET);
  int size = 4096;
  union ktr_stuff *buf = malloc(size);
  int sparesize = 4096;
  union ktr_stuff *sparebuf = malloc(sparesize);
  struct ktr_header kth, sparekth;

  while (!read_ktrace_entry(tracefd, &kth, &buf, &size)) {
    switch (kth.ktr_type) {
    case KTR_SYSCALL:
      {
        const char *name = syscalls_freebsd[buf->sc.ktr_code];
        pid_t child = kth.ktr_pid;
        if (!strcmp(name, "open")) {
          long flags = buf->sc.ktr_args[1];
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          int fd = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          if (flags & O_DIRECTORY) {
            debugprintf("%d: opendir('%s') -> %d\n", child, arg, fd);
            model_opendir(m, model_cwd(m, child), arg, child, fd);
          } else if (flags & (O_WRONLY | O_RDWR)) {
            debugprintf("%d: %s('%s', 'w') -> %d\n", child, name, arg, fd);
            if (fd >= 0) model_creat(m, model_cwd(m, child), arg);
          } else {
            debugprintf("%d: %s('%s', 'r') -> %d\n", child, name, arg, fd);
            if (fd >= 0) {
              struct inode *i = model_stat(m, model_cwd(m, child), arg);
              if (i && i->type == is_file) i->is_read = true;
            }
          }
          free(arg);
        } else if (!strcmp(name, "getdents") || !strcmp(name, "getdirentries")) {
          int fd = buf->sc.ktr_args[0];
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          int retval = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s(%d) -> %d\n", child, name, fd, retval);
          struct inode *i = lookup_fd(m, child, fd);
          if (i) i->is_read = true;
        } else if (!strcmp(name, "vfork") || !strcmp(name, "fork")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          int retval = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s() -> %d\n", child, name, retval);
          if (retval > 0) model_chdir(m, model_cwd(m, child), ".", retval);
        } else if (!strcmp(name, "execve")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s')\n", child, name, arg);
          struct inode *i = model_stat(m, model_cwd(m, child), arg);
          if (i && i->type == is_file) i->is_read = true;
          free(arg);
        } else if (!strcmp(name, "chdir")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          int retval = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s') -> %d\n", child, name, arg, retval);
          if (retval >= 0) {
            model_chdir(m, model_cwd(m, child), arg, child);
          }
          free(arg);
        } else if (!strcmp(name, "stat")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          int retval = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s') -> %d\n", child, name, arg, retval);
          if (retval >= 0) {
            struct inode *i = model_stat(m, model_cwd(m, child), arg);
            if (i && i->type == is_file) i->is_read = true;
          }
          free(arg);
        } else if (!strcmp(name, "lstat") || !strcmp(name, "readlink")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          int retval = read_ktrace_sysret(tracefd, child, &sparekth,
                                          &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s') -> %d\n", child, name, arg, retval);
          if (retval >= 0) {
            struct inode *i = model_lstat(m, model_cwd(m, child), arg);
            if (i && i->type != is_directory) i->is_read = true;
          }
          free(arg);
        } else {
          /* printf("CALL %s", name); */
          /* for (int i=0; i<buf->sc.ktr_narg; i++) { */
          /*   printf(" %x", (int)buf->sc.ktr_args[i]); */
          /* } */
          /* printf("\n"); */
        }
      }
      break;
    case KTR_NAMEI:
      buf->ni[kth.ktr_len] = 0;
      /* printf("NAMI %s\n", buf->ni); */
      break;
    case KTR_SYSRET:
      {
        /* printf("%s RETURNS: %d\n", */
        /*        syscalls_freebsd[buf->sr.ktr_code], */
        /*        (int)buf->sr.ktr_retval); */
      }
      break;
    default:
      printf("WHATEVER\n");
    }
  }
  free(buf);
}

#endif
