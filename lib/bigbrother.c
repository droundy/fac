#define _XOPEN_SOURCE 700

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
#include <stdarg.h>
#include <error.h>

#include "syscalls.h"
#include "bigbrother.h"

static const int debug_output = 0;

static void debugprintf(const char *format, ...) {
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
    /* the following seems like a good idea, but causes trouble when
       the entire source tree happens to reside in /tmp.  */
    //if (memcmp(path, "/tmp/", 5) == 0) return 0;
    if (memcmp(path, "/proc/", 6) == 0) return 0;
  }
  return 1;
}

static void ptrace_syscall(pid_t pid) {
  //debugprintf("\tcalling ptrace_syscall on %d\n", pid);
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
  realpath(path, path_buffer);
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

static int save_syscall_access(pid_t child,
                               listset **read_from_directories,
                               listset **read_from_files,
                               listset **written_to_files,
                               listset **deleted_files) {
  struct user_regs_struct regs;
  int syscall;

  if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
    debugprintf("ERROR PTRACING %d!\n", child);
    error(1, errno, "error getting registers for %d...", child);
    exit(1);
  }
  syscall = regs.orig_rax;
  debugprintf(">> %d >> %s()\n", child, syscalls[syscall]);

  if (write_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, write_fd[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        debugprintf("W: %s(%s)\n", syscalls[syscall], filename);
        insert_to_listset(written_to_files, filename);
        delete_from_listset(read_from_files, filename);
        delete_from_listset(deleted_files, filename);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, read_fd[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename) &&
          !is_in_listset(*written_to_files, filename)) {
        debugprintf("R: %s(%s)\n", syscalls[syscall], filename);
        insert_to_listset(read_from_files, filename);
      } else {
        debugprintf("R~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (readdir_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, readdir_fd[syscall]);
    debugprintf("!!!!!!!!! Got readdir with fd %d\n", fd);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        debugprintf("readdir: %s(%s)\n", syscalls[syscall], filename);
        insert_to_listset(read_from_directories, filename);
      } else {
        debugprintf("readdir~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, read_string[syscall]));
    if (interesting_path(arg) && !access(arg, R_OK) &&
        !is_in_listset(*written_to_files, arg)) {
      debugprintf("R: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(read_from_files, arg);
    } else {
      debugprintf("R~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (write_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, write_string[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(written_to_files, arg);
      delete_from_listset(deleted_files, arg);
      delete_from_listset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (unlink_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, unlink_string[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(deleted_files, arg);
      delete_from_listset(written_to_files, arg);
      delete_from_listset(read_from_files, arg);
      delete_from_listset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (unlinkat_string[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 0) /* dirfd */,
                               get_syscall_arg(&regs, 1) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(deleted_files, arg);
      delete_from_listset(written_to_files, arg);
      delete_from_listset(read_from_files, arg);
      delete_from_listset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (renameat_string[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 2) /* dirfd */,
                               get_syscall_arg(&regs, 3) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls[syscall], arg);
      insert_to_listset(written_to_files, arg);
      delete_from_listset(deleted_files, arg);
      delete_from_listset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  return syscall;
}

pid_t wait_for_syscall(int *num_programs) {
  pid_t child = 0;
  int status = 0;
  while (1) {
    //debugprintf("waiting for any syscall...\n");
    child = waitpid(-1, &status, __WALL);
    if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
      return child;
    } else if (WIFEXITED(status)) {
      //debugprintf("got an exit from %d...\n", child);
      if (--(*num_programs) <= 0) return -WEXITSTATUS(status);
      continue; /* no need to do anything more for this guy */
    } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      debugprintf("\nexeced!!! %d from %d\n", newpid, child);
    } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
      debugprintf("vfork is done in %d\n", child);
    } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      debugprintf("\ncloned %d from %d!!!\n", newpid, child);
      waitpid(newpid, 0, __WALL);
      if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
        debugprintf("error ptracing setoptions n %d\n", newpid);
      }
      //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
      num_programs++;

      ptrace_syscall(newpid);
    } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      //debugprintf("\nvforked %d from %d!!!\n", newpid, child);
      waitpid(newpid, 0, __WALL);
      if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
        debugprintf("error ptracing setoptions n %d\n", newpid);
      }
      //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
      (*num_programs)++;

      ptrace_syscall(newpid);
    } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
      pid_t newpid;
      ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
      //debugprintf("\nforked %d from %d!!!\n", newpid, child);
      waitpid(newpid, 0, __WALL);
      if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
        debugprintf("error ptracing setoptions n %d\n", newpid);
      }
      //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
      (*num_programs)++;

      ptrace_syscall(newpid);
    } else if (WIFSIGNALED(status)) {
      debugprintf("foo signaled!!! %d\n", child);
    } else if (WIFCONTINUED(status)) {
      debugprintf("foo continued!!! %d\n", child);
    } else {
      debugprintf("I do not understand this event %d\n\n", status);
    }
    ptrace_syscall(child); // keep going!
  }
}

int bigbrother_process(const char *workingdir,
                       char **args,
                       listset **read_from_directories,
                       listset **read_from_files,
                       listset **written_to_files,
                       listset **deleted_files) {
  pid_t child = fork();
  if (child == 0) {
    if (workingdir && chdir(workingdir) != 0) return -1;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
  } else {
    int num_programs = 1;
    waitpid(-1, 0, __WALL);
    ptrace(PTRACE_SETOPTIONS, child, 0, my_ptrace_options);
    ptrace_syscall(child); // run until a sycall is attempted

    while (num_programs > 0) {
      pid_t child = 0;
      int status, syscall;
    look_for_syscall:
      child = wait_for_syscall(&num_programs);
      if (child <= 0) return -child;

      syscall = save_syscall_access(child,
                                    read_from_directories,
                                    read_from_files,
                                    written_to_files,
                                    deleted_files);

      if (is_wait_or_exit[syscall]) {
        /* These syscalls may wait on a child process, so we cannot
           wait for their return, since this may not happen if stop
           processing the child processes.  So my simple
           (non-threaded) approach is just to ignore their return
           value. */
        debugprintf("%d GOT waiting or exiting syscall\n", child);
        ptrace(PTRACE_SYSCALL, child, 0, 0); // ignore return value
        goto look_for_syscall;
      }

      ptrace_syscall(child); // run the syscall to its finish

      while (1) {
        //debugprintf("waiting for syscall from %d...\n", child);
        waitpid(child, &status, __WALL);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
          break;
        } else if (WIFEXITED(status)) {
          //debugprintf("we got an exit from %d...\n", child);
          if (--num_programs <= 0) return WEXITSTATUS(status);
          goto look_for_syscall; // no point looking any longer!
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nexeced!!! %d from %d\n", newpid, child);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
          debugprintf("vfork is done in %d\n", child);
          ptrace_syscall(child); // skip over return value of vfork
          waitpid(child, &status, __WALL);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          debugprintf("\ncloned %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("now waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nvforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
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


static int save_syscall_access_arrayset(pid_t child,
                                        arrayset *read_from_directories,
                                        arrayset *read_from_files,
                                        arrayset *written_to_files,
                                        arrayset *deleted_files) {
  struct user_regs_struct regs;
  int syscall;

  if (ptrace(PTRACE_GETREGS, child, NULL, &regs) == -1) {
    debugprintf("ERROR PTRACING %d!\n", child);
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
        debugprintf("W: %s(%s)\n", syscalls[syscall], filename);
        insert_to_arrayset(written_to_files, filename);
        delete_from_arrayset(read_from_files, filename);
        delete_from_arrayset(deleted_files, filename);
      } else {
        debugprintf("W~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, read_fd[syscall]);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename) &&
          !is_in_arrayset(written_to_files, filename)) {
        debugprintf("R: %s(%s)\n", syscalls[syscall], filename);
        insert_to_arrayset(read_from_files, filename);
      } else {
        debugprintf("R~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (readdir_fd[syscall] >= 0) {
    int fd = get_syscall_arg(&regs, readdir_fd[syscall]);
    debugprintf("!!!!!!!!! Got readdir with fd %d\n", fd);
    if (fd >= 0) {
      char *filename = (char *)malloc(PATH_MAX);
      identify_fd(filename, child, fd);
      if (interesting_path(filename)) {
        debugprintf("readdir: %s(%s)\n", syscalls[syscall], filename);
        insert_to_arrayset(read_from_directories, filename);
      } else {
        debugprintf("readdir~ %s(%s)\n", syscalls[syscall], filename);
      }
      free(filename);
    }
  }
  if (read_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, read_string[syscall]));
    if (interesting_path(arg) && !access(arg, R_OK) &&
        !is_in_arrayset(written_to_files, arg)) {
      debugprintf("R: %s(%s)\n", syscalls[syscall], arg);
      insert_to_arrayset(read_from_files, arg);
    } else {
      debugprintf("R~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (write_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, write_string[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls[syscall], arg);
      insert_to_arrayset(written_to_files, arg);
      delete_from_arrayset(deleted_files, arg);
      delete_from_arrayset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (unlink_string[syscall] >= 0) {
    char *arg = read_a_path(child, get_syscall_arg(&regs, unlink_string[syscall]));
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls[syscall], arg);
      insert_to_arrayset(deleted_files, arg);
      delete_from_arrayset(written_to_files, arg);
      delete_from_arrayset(read_from_files, arg);
      delete_from_arrayset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (unlinkat_string[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 0) /* dirfd */,
                               get_syscall_arg(&regs, 1) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("D: %s(%s)\n", syscalls[syscall], arg);
      insert_to_arrayset(deleted_files, arg);
      delete_from_arrayset(written_to_files, arg);
      delete_from_arrayset(read_from_files, arg);
      delete_from_arrayset(read_from_directories, arg);
    } else {
      debugprintf("D~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  if (renameat_string[syscall] >= 0) {
    char *arg = read_a_path_at(child,
                               get_syscall_arg(&regs, 2) /* dirfd */,
                               get_syscall_arg(&regs, 3) /* path */);
    if (interesting_path(arg) && !access(arg, W_OK)) {
      debugprintf("W: %s(%s)\n", syscalls[syscall], arg);
      insert_to_arrayset(written_to_files, arg);
      delete_from_arrayset(deleted_files, arg);
      delete_from_arrayset(read_from_files, arg);
    } else {
      debugprintf("W~ %s(%s)\n", syscalls[syscall], arg);
    }
    free(arg);
  }
  return syscall;
}


int bigbrother_process_arrayset(const char *workingdir,
                                char **args,
                                pid_t *store_child_pid_here,
                                arrayset *read_from_directories,
                                arrayset *read_from_files,
                                arrayset *written_to_files,
                                arrayset *deleted_files) {
  initialize_arrayset(read_from_directories);
  initialize_arrayset(read_from_files);
  initialize_arrayset(written_to_files);
  initialize_arrayset(deleted_files);

  pid_t child = fork();
  if (child == 0) {
    if (workingdir && chdir(workingdir) != 0) return -1;
    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
  } else {
    *store_child_pid_here = child;
    int num_programs = 1;
    waitpid(-1, 0, __WALL);
    ptrace(PTRACE_SETOPTIONS, child, 0, my_ptrace_options);
    ptrace_syscall(child); // run until a sycall is attempted

    while (num_programs > 0) {
      pid_t child = 0;
      int status, syscall;
    look_for_syscall:
      child = wait_for_syscall(&num_programs);
      if (child <= 0) return -child;

      syscall = save_syscall_access_arrayset(child,
                                             read_from_directories,
                                             read_from_files,
                                             written_to_files,
                                             deleted_files);

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
        //debugprintf("waiting for syscall from %d...\n", child);
        waitpid(child, &status, __WALL);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
          break;
        } else if (WIFEXITED(status)) {
          //debugprintf("we got an exit from %d...\n", child);
          if (--num_programs <= 0) return WEXITSTATUS(status);
          goto look_for_syscall; // no point looking any longer!
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nexeced!!! %d from %d\n", newpid, child);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE<<8))) {
          debugprintf("vfork is done in %d\n", child);
          ptrace_syscall(child); // skip over return value of vfork
          waitpid(child, &status, __WALL);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          debugprintf("\ncloned %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(newpid);
        } else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK<<8))) {
          pid_t newpid;
          ptrace(PTRACE_GETEVENTMSG, child, 0, &newpid);
          //debugprintf("\nvforked %d from %d!!!\n", newpid, child);
          waitpid(newpid, 0, __WALL);
          //debugprintf("waitpid %d worked!!!\n", newpid);
          if (ptrace(PTRACE_SETOPTIONS, newpid, 0, my_ptrace_options)) {
            debugprintf("error ptracing setoptions n %d\n", newpid);
          }
          //debugprintf("ptrace setoptions %d worked!!!\n", newpid);
          num_programs++;

          ptrace_syscall(child);
          ptrace_syscall(newpid);
          goto look_for_syscall;
        } else {
          debugprintf("I do not understand this event %d\n\n", status);
          /* exit(1); */
        }
        ptrace_syscall(child); // we don't understand id, so keep trying
      }
      ptrace_syscall(child);

    }
  }
  return 0;
}
