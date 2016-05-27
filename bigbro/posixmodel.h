#ifndef POSIXMODEL_H
#define POSIXMODEL_H

#include "intmap.c"

#include <sys/types.h>

#include <stdbool.h>

enum inode_type {
  not_here = 0, is_file, is_directory, is_symlink, is_dir_or_symlink
};

struct inode {
  struct hash_entry e;
  enum inode_type type;
  struct inode *parent;
  bool is_written;
  bool is_read;
  union {
    struct hash_table children;
    char *readlink;
  } c; /* c for "contents" */
  char name[];
};

struct posixmodel {
  struct inode *root;
  int num_fds;
  struct intmap processes; // holds mappings from threads to processes
};

static void free_inode(struct inode *);

static void init_posixmodel(struct posixmodel *m);
static void free_posixmodel(struct posixmodel *m);

static struct inode *lookup_fd(struct posixmodel *m, pid_t pid, int fd);

static void model_dup2(struct posixmodel *m, pid_t pid, int fdorig, int fdtarget);

static void model_fork(struct posixmodel *m, pid_t parent, pid_t child);

static char *model_realpath(struct inode *i);
static struct inode *model_cwd(struct posixmodel *m, pid_t pid);
static struct inode *model_lstat(struct posixmodel *m, struct inode *cwd,
                                 const char *path0);
static struct inode *model_stat(struct posixmodel *m, struct inode *cwd,
                                const char *path0);

static int model_chdir(struct posixmodel *m, struct inode *cwd,
                       const char *dir, pid_t pid);
static void model_newthread(struct posixmodel *m, pid_t parent, pid_t child);

static void model_unlink(struct posixmodel *m, struct inode *cwd, const char *f);

static void model_symlink(struct posixmodel *m, struct inode *parent,
                          const char *name, const char *contents);

static int model_opendir(struct posixmodel *m, struct inode *cwd,
                         const char *dir, pid_t pid, int fd);
static void model_close(struct posixmodel *m, pid_t pid, int fd);

static void model_rename(struct posixmodel *m, struct inode *cwd,
                         const char *from, const char *to);

static struct inode *model_creat(struct posixmodel *m, struct inode *cwd,
                                 const char *path);

#endif
