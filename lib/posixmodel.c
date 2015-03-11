#define _XOPEN_SOURCE 700

#include "iterablehash.h"
#include "posixmodel.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
  inode->is_written = false;
  inode->is_read = false;
  if (parent) add_to_hash(&parent->c.children, &inode->e);
  init_hash_table(&inode->c.children, 5);
  return inode;
}

struct inode *alloc_symlink(struct inode *parent, const char *name,
                            const char *contents, int size) {
  struct inode *inode = malloc(sizeof(struct inode) + strlen(name) + 1);
  strcpy(inode->name, name);
  inode->e.key = inode->name;
  inode->e.next = 0;
  inode->parent = parent;
  inode->type = is_symlink;
  inode->is_written = false;
  inode->is_read = false;
  add_to_hash(&parent->c.children, &inode->e);

  if (contents) {
    inode->c.readlink = malloc(strlen(contents)+1);
    strcpy(inode->c.readlink, contents);
  } else {
    char *path = model_realpath(parent);
    path = realloc(path, strlen(path) + 1 + strlen(name));
    strcat(path, "/");
    strcat(path, name);
    if (!size) {
      struct stat st;
      lstat(path, &st);
      size = st.st_size;
    }
    inode->c.readlink = malloc(size+2);
    int linklen = readlink(path, inode->c.readlink, size+2);
    while (linklen == size+2) {
      /* This means our buffer filled, which could happen if the
         symlink is modified after we run lstat but before we run
         readlink.  Probably we don't need to handle this race
         condition, but it still seems like a good idea. */
      size += 100;
      inode->c.readlink = realloc(inode->c.readlink, size+2);
      linklen = readlink(path, inode->c.readlink, size+2);
    }
    if (linklen != -1) {
      inode->c.readlink[linklen] = 0; /* add the extra zero readlink may omit */
      free(path);
    } else {
      /* The path is a symlink, but we cannot read it, or it was
         changed to not be a symlink after we ran lstat...) */
      free(inode->c.readlink);
      free(inode);
      free(path);
      return 0;
    }
  }
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
    struct inode *child = (struct inode *)lookup_in_hash(&cwd->c.children, tmp);
    if (!child) {
      char *path = model_realpath(cwd);
      path = realloc(path, strlen(path) + 1 + strlen(tmp));
      strcat(path, "/");
      strcat(path, tmp);
      struct stat st;
      if (!lstat(path, &st) && S_ISLNK(st.st_mode)) {
        child = alloc_symlink(cwd, tmp, 0, st.st_size);
        if (child) {
          child->is_read = true; /* we read any symlinks that we dereference */
          if (!done) {
            char *newpath = malloc(strlen(child->c.readlink) + 1 + strlen(dir) + 1);
            sprintf(newpath, "%s/%s", child->c.readlink, dir + i + 1);
            printf("we are going into %s\n", newpath);
            struct inode *in = interpret_path_as_directory(cwd, newpath);
            free(newpath);
            return in;
          } else {
            return interpret_path_as_directory(cwd, child->c.readlink);
          }
        }
      }
      child = alloc_directory(cwd, tmp);
      free(path);
    }
    free(tmp);
    dir += i+1;

    if (child->type == is_symlink) {
      child->is_read = true; /* we read any symlinks that we dereference */
      if (done) {
        return interpret_path_as_directory(cwd, child->c.readlink);
      } else {
        char *newpath = malloc(strlen(child->c.readlink) + 1 + strlen(dir) + 1);
        sprintf(newpath, "%s/%s", child->c.readlink, dir);
        struct inode *in = interpret_path_as_directory(cwd, newpath);
        free(newpath);
        return in;
      }
    }
    cwd = child;
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
