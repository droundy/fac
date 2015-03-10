#define _XOPEN_SOURCE 700

#include "iterablehash.h"
#include "posixmodel.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

static const int CWD = -1;
static struct inode *model_root = 0;

struct inode_pid_fd {
  struct inode *inode;
  pid_t pid;
  int fd;
};

static int num_fds = 0;
static struct inode_pid_fd *open_stuff = 0;

struct inode *lookup_fd(pid_t pid, int fd) {
  for (int i=0; i<num_fds; i++) {
    if (open_stuff[i].pid == pid && open_stuff[i].fd == fd) {
      return open_stuff[i].inode;
    }
  }
  return 0;
}

void create_fd(pid_t pid, int fd, struct inode *inode) {
  for (int i=0; i<num_fds; i++) {
    if (open_stuff[i].pid == pid && open_stuff[i].fd == fd) {
      open_stuff[i].inode = inode;
    }
  }
  open_stuff = realloc(open_stuff, (num_fds+1)*sizeof(struct inode_pid_fd));
  open_stuff[num_fds].inode = inode;
  open_stuff[num_fds].fd = fd;
  open_stuff[num_fds].pid = pid;
  num_fds += 1;
}

void close_fd(pid_t pid, int fd) {
  for (int i=0; i<num_fds; i++) {
    if (open_stuff[i].pid == pid && open_stuff[i].fd == fd) {
      for (int j=i+1;j<num_fds;j++) {
        open_stuff[j-1] = open_stuff[j];
      }
      return;
    }
  }
  num_fds -= 1;
}

struct inode *alloc_directory(struct inode *parent, const char *name) {
  struct inode *inode = malloc(sizeof(struct inode) + strlen(name) + 1);
  strcpy(inode->name, name);
  inode->e.key = inode->name;
  inode->e.next = 0;
  inode->parent = parent;
  inode->type = is_directory;
  init_hash_table(&inode->children, 5);
  return inode;
}

static void init_root() {
  if (!model_root) model_root = alloc_directory(0, "/");
}

struct inode *interpret_path_as_directory(struct inode *cwd, const char *dir) {
  init_root();
  assert(dir[0] == '/' || cwd);
  if (dir[0] == '/') {
    /* absolute paths can be interpreted as relative paths to root */
    cwd = model_root;
    dir++;
  }
  bool done = false;
  while (!done) {
    /* FIXME: the following is broke under symlinks.  I am leaving
       symlink implementation for later. */
    if (cwd->type != is_directory) return 0;
    char *tmp = malloc(strlen(dir)+1);
    int i;
    for (i=0;dir[i] && dir[i] != '/';i++) {
      tmp[i] = dir[i];
    }
    if (!dir[i]) done = true;
    tmp[i] = 0;
    struct inode *child = (struct inode *)lookup_in_hash(&cwd->children, tmp);
    if (!child) child = alloc_directory(cwd, tmp);
    free(tmp);
    cwd = child;
    dir += i+1;
  }

  return cwd;
}

char *model_realpath(struct inode *i) {
  assert(i);
  if (i->parent == 0) return strdup("/");

  char *parent_path = model_realpath(i->parent);
  char *mypath = realloc(parent_path, strlen(parent_path)+1+strlen(i->e.key)+1);
  if (i->parent->parent) strcat(mypath, "/");
  strcat(mypath, i->e.key);
  return mypath;
}

struct inode *model_cwd(pid_t pid) {
  return lookup_fd(pid, CWD);
}

int model_chdir(struct inode *cwd, const char *dir, pid_t pid) {
  struct inode *thisdir = interpret_path_as_directory(cwd, dir);
  if (thisdir) {
    create_fd(pid, CWD, thisdir);
    return 0;
  }
  return -1;
}
