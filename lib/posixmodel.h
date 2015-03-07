#ifndef POSIXMODEL_H
#define POSIXMODEL_H

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

/* void model_move(const char *from, const char *to); */
/* void model_mkdir(const char *from); */
/* void model_stat(const char *from); */
/* void model_open_read(const char *from, pid_t pid, int fdresult); */
/* void model_open_write(const char *from); */

#endif
