#ifndef POSIXMODEL_H
#define POSIXMODEL_H

#include <sys/types.h>

enum inode_type {
  not_here = 0, is_file, is_directory, is_symlink, is_dir_or_symlink
};

struct inode {
  struct hash_entry e;
  enum inode_type type;
  struct inode *parent;
  struct hash_table children;
  char name[];
};

struct inode *lookup_fd(pid_t pid, int fd);

char *model_realpath(struct inode *i);
struct inode *model_cwd(pid_t pid);

int model_chdir(struct inode *cwd, const char *dir, pid_t pid);

#endif
