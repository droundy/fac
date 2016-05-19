#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1

#include "bigbro.h"
#include "posixmodel.c"
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

static void read_ktrace(int ktrfd, struct posixmodel *m);

int bigbro(const char *workingdir, pid_t *child_ptr,
           int stdouterrfd, char **args,
           char ***readdirs,
           char ***reads,
           char ***writes) {
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

  model_output(&m, readdirs, reads, writes);

  if (WIFEXITED(status)) return -WEXITSTATUS(status);
  return 1;
}

union ktr_stuff {
  struct ktr_syscall sc;
  struct ktr_sysret sr;
  char ni[4096];
};

static int read_ktrace_entry(int tracefd, struct ktr_header *kth,
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

static char *read_ktrace_namei(int tracefd, pid_t pid, struct ktr_header *kth,
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

static int read_ktrace_sysret(int tracefd, pid_t pid, struct ktr_header *kth,
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

static void read_ktrace(int tracefd, struct posixmodel *m) {
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
          if (retval > 0) model_fork(m, child, retval);
        } else if (!strcmp(name, "execve")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s')\n", child, name, arg);
          struct inode *i = model_stat(m, model_cwd(m, child), arg);
          if (i && i->type == is_file) i->is_read = true;
          free(arg);
        } else if (!strcmp(name, "creat") || !strcmp(name, "truncate")) {
          off_t where_from = lseek(tracefd, 0, SEEK_CUR);
          char *arg = read_ktrace_namei(tracefd, child, &sparekth,
                                        &sparebuf, &sparesize);
          lseek(tracefd, where_from, SEEK_SET);
          debugprintf("%d: %s('%s')\n", child, name, arg);
          model_creat(m, model_cwd(m, child), arg);
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
