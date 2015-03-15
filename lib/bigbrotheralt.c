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

static inline void error(int retval, int errno, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "error: ");
  vfprintf(stderr, format, args);
  if (errno) fprintf(stderr, "\n  %s\n", strerror(errno));
  va_end(args);
  exit(retval);
}

#ifdef __linux__

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <linux/limits.h>
#include <errno.h>

#include <stdint.h>

#include "linux-syscalls.h"

static const int debug_output = 1;

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
      printf("%ld: forked from %d\n", pid, child);
      model_chdir(m, model_cwd(m, child), ".", pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_VFORK << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      printf("%ld: vforked from %d\n", pid, child);
      model_chdir(m, model_cwd(m, child), ".", pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_CLONE << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      printf("%ld: cloned from %d\n", pid, child);
      model_newthread(m, child, pid);
    } else if (WIFSTOPPED(status) && (status>>8) == (SIGTRAP | PTRACE_EVENT_EXEC << 8)) {
      unsigned long pid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &pid);
      printf("%ld: execed from %d\n", pid, child);
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
  struct i386_user_regs_struct *i386_regs = malloc(sizeof(struct i386_user_regs_struct));
  if (regs->cs == 0x23) {
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
  if (!name) return -1;

  //debugprintf("%s(?)\n",name);

  if (!strcmp(name, "open")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    long flags = get_syscall_arg(regs, 1);
    int fd = wait_for_return_value(m, child);
    if (flags & O_DIRECTORY) {
      debugprintf("%d: opendir('%s') -> %d\n", child, arg, fd);
      model_opendir(m, model_cwd(m, child), arg, child, fd);
    } else if (flags & (O_WRONLY | O_RDWR)) {
      debugprintf("%d: open('%s', 'w') -> %d\n", child, arg, fd);
      if (fd >= 0) {
        struct inode *i = model_lstat(m, model_cwd(m, child), arg);
        if (i) i->is_written = true;
      }
    } else {
      debugprintf("%d: open('%s', 'r') -> %d\n", child, arg, fd);
      struct inode *i = model_lstat(m, model_cwd(m, child), arg);
      if (i) {
        debugprintf("%d: has read %s\n", child, model_realpath(i));
        i->is_read = true;
      }
    }
    free(arg);
  } else if (!strcmp(name, "creat")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int fd = wait_for_return_value(m, child);
    debugprintf("%d: creat('%s') -> %d\n", child, arg, fd);
    if (fd >= 0) {
      struct inode *i = model_lstat(m, model_cwd(m, child), arg);
      if (i) i->is_written = true;
    }
    free(arg);
  } else if (!strcmp(name, "lstat")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: lstat('%s') -> %d\n", child, arg, retval);
    struct inode *i = model_lstat(m, model_cwd(m, child), arg);
    if (i && i->type != is_directory) {
      debugprintf("%d: has read %s\n", child, model_realpath(i));
      i->is_read = true;
    }
    free(arg);
  } else if (!strcmp(name, "stat")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: stat('%s') -> %d\n", child, arg, retval);
    struct inode *i = model_stat(m, model_cwd(m, child), arg);
    if (i && i->type != is_directory) {
      debugprintf("%d: has read %s\n", child, model_realpath(i));
      i->is_read = true;
    }
    free(arg);
  } else if (!strcmp(name, "execve")) {
    char *arg = read_a_string(child, get_syscall_arg(regs, 0));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: execve('%s') -> %d\n", child, arg, retval);
    struct inode *i = model_stat(m, model_cwd(m, child), arg);
    if (i) {
      debugprintf("%d: has read %s\n", child, model_realpath(i));
      i->is_read = true;
    }
    free(arg);
  } else if (!strcmp(name, "rename")) {
    char *from = read_a_string(child, get_syscall_arg(regs, 0));
    char *to = read_a_string(child, get_syscall_arg(regs, 1));
    int retval = wait_for_return_value(m, child);
    debugprintf("%d: rename('%s', '%s') -> %d\n", child, from, to, retval);
    model_rename(m, model_cwd(m, child), from, to);
    free(from);
    free(to);
  } else if (!strcmp(name, "close")) {
    int fd = get_syscall_arg(regs, 0);
    debugprintf("%d: close(%d)\n", child, fd);
    model_close(m, child, fd);
    wait_for_return_value(m, child);
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

    struct posixmodel m;
    init_posixmodel(&m);
    {
      char *cwd = getcwd(0,0);
      model_chdir(&m, 0, cwd, firstborn);
      free(cwd);
    }

    while (1) {
      pid_t child = wait_for_syscall(&m, firstborn);
      if (child <= 0) {
        debugprintf("Returning with exit value %d\n", -child);
        model_output(&m, read_from_directories, read_from_files, written_to_files);
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

#include <sys/syscall.h>
#include "syscalls-freebsd.h"
int nsyscalls = sizeof(syscallnames)/sizeof(syscallnames[0]);

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
			KTRFAC_SYSCALL | KTRFAC_NAMEI | KTRFAC_INHERIT,
			getpid());
    if (retval) error(1, errno, "ktrace gives %d", retval);
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
  printf("dumping trace info from %s... %d\n",
	 namebuf, (int)lseek(tracefd, 0, SEEK_END));
  /* unlink(namebuf); */
  free(namebuf);

  lseek(tracefd, 0, SEEK_SET);
  int size = 4096;
  char *buf = malloc(size);
  struct ktr_header kth;
  while (read(tracefd, &kth, sizeof(struct ktr_header)) == sizeof(struct ktr_header)) {
    if (kth.ktr_len+1 > size) {
      buf = realloc(buf, kth.ktr_len+1);
      size = kth.ktr_len+1;
    }
    if (read(tracefd, buf, kth.ktr_len) != kth.ktr_len) {
      printf("error reading.\n");
      break;
    }
    switch (kth.ktr_type) {
    case KTR_SYSCALL:
      {
	struct ktr_syscall *sc = (struct ktr_syscall *)buf;
	printf("CALL %s\n", syscallnames[sc->ktr_code]);
      }
      break;
    case KTR_NAMEI:
      buf[kth.ktr_len] = 0;
      printf("NAMI %s\n", buf);
      break;
    default:
      printf("WHATEVER\n");
    }
  }
  free(buf);

  int status = 0;
  waitpid(-firstborn, &status, 0);
  if (WIFEXITED(status)) return -WEXITSTATUS(status);
  return 1;
}
 
#endif
