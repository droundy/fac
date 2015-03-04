#include "iterablehash.h"
#include "posixmodel.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

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

void model_chdir(struct inode *cwd, const char *dir) {
  init_root();
  assert(dir[0] == '/' || cwd);
}

void model_move(struct inode *cwd, const char *from, const char *to) {
  init_root();
}

void model_mkdir(struct inode *cwd, const char *from) {
  init_root();
}

void model_stat(struct inode *cwd, const char *from) {
  init_root();
}

void model_open_read(struct inode *cwd, const char *from, int fdresult) {
  init_root();
}

void model_open_write(struct inode *cwd, const char *from) {
  init_root();
}

