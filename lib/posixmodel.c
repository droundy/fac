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

static const int CWD = -100; /* this must be fixed, is AT_FDCWD in fcntl.h */

struct inode *lookup_fd(struct posixmodel *m, pid_t pid, int fd) {
  for (int i=0; i<m->num_fds; i++) {
    if (m->open_stuff[i].pid == pid && m->open_stuff[i].fd == fd) {
      return m->open_stuff[i].inode;
    }
    if (m->open_stuff[i].pid == pid && !m->open_stuff[i].inode) {
      // This indicates we are a separate thread, and the fd field
      // holds the "real" pid!
      return lookup_fd(m, m->open_stuff[i].fd, fd);
    }
  }
  return 0;
}

void create_fd(struct posixmodel *m, pid_t pid, int fd, struct inode *inode) {
  for (int i=0; i<m->num_fds; i++) {
    if (m->open_stuff[i].pid == pid && m->open_stuff[i].fd == fd) {
      m->open_stuff[i].inode = inode;
    }
  }
  m->open_stuff = realloc(m->open_stuff, (m->num_fds+1)*sizeof(struct inode_pid_fd));
  m->open_stuff[m->num_fds].inode = inode;
  m->open_stuff[m->num_fds].fd = fd;
  m->open_stuff[m->num_fds].pid = pid;
  m->num_fds += 1;
}

void model_fork(struct posixmodel *m, pid_t parent, pid_t child) {
  int num_fd_in_parent = 0;
  for (int i=0; i<m->num_fds; i++) {
    if (m->open_stuff[i].pid == parent) num_fd_in_parent++;
  }
  m->open_stuff = realloc(m->open_stuff, (m->num_fds+num_fd_in_parent)*sizeof(struct inode_pid_fd));
  int j = m->num_fds;
  for (int i=0; i<m->num_fds; i++) {
    if (m->open_stuff[i].pid == parent) {
      m->open_stuff[j] = m->open_stuff[i];
      m->open_stuff[j].pid = child;
      j++;
    }
  }
  m->num_fds += num_fd_in_parent;
}

void model_dup2(struct posixmodel *m, pid_t pid, int fdorig, int fdtarget) {
  struct inode *i = lookup_fd(m, pid, fdorig);
  if (i) create_fd(m, pid, fdtarget, i);
}

void model_close(struct posixmodel *m, pid_t pid, int fd) {
  /* printf("closing [%d]: %d\n", pid, fd); */
  /* for (int i=0; i<m->num_fds; i++) { */
  /*   printf("[%d]: %d = %s\n", m->open_stuff[i].pid, */
  /*          m->open_stuff[i].fd, model_realpath(m->open_stuff[i].inode)); */
  /* } */
  for (int i=0; i<m->num_fds; i++) {
    if (m->open_stuff[i].pid == pid && m->open_stuff[i].fd == fd) {
      for (int j=i+1;j<m->num_fds;j++) {
        m->open_stuff[j-1] = m->open_stuff[j];
      }
      m->num_fds -= 1;
      return;
    }
  }
}

struct inode *alloc_file(struct inode *parent, const char *name) {
  if (parent) {
    struct inode *inode = (struct inode *)lookup_in_hash(&parent->c.children, name);
    if (inode) return inode;
  }
  struct inode *inode = malloc(sizeof(struct inode) + strlen(name) + 1);
  strcpy(inode->name, name);
  inode->e.key = inode->name;
  inode->e.next = 0;
  inode->parent = parent;
  inode->type = is_file;
  inode->is_written = false;
  inode->is_read = false;
  if (parent) add_to_hash(&parent->c.children, &inode->e);
  return inode;
}

struct inode *alloc_directory(struct inode *parent, const char *name) {
  struct inode *inode = alloc_file(parent, name);
  inode->type = is_directory;
  init_hash_table(&inode->c.children, 5);
  return inode;
}

void init_posixmodel(struct posixmodel *m) {
  m->root = alloc_directory(0, "/");
  m->num_fds = 0;
  m->open_stuff = 0;
}

void free_posixmodel(struct posixmodel *m) {
  free_inode(m->root);
  free(m->open_stuff);
}

struct inode *alloc_symlink(struct posixmodel *m, struct inode *parent, const char *name,
                            const char *contents, int size) {
  struct inode *inode = alloc_file(parent, name);
  inode->type = is_symlink;

  if (contents) {
    inode->c.readlink = malloc(strlen(contents)+1);
    strcpy(inode->c.readlink, contents);
  } else {
    char *path = model_realpath(parent);
    path = realloc(path, strlen(path) + 1 + strlen(name)+1);
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

struct inode *interpret_path_as_directory(struct posixmodel *m,
                                          struct inode *cwd, const char *dir) {
  if (dir[0] != '/' && !cwd) {
    fprintf(stderr, "uh oh trouble with %s\n", dir);
  }
  assert(dir[0] == '/' || cwd);
  if (dir[0] == '/') {
    /* absolute paths can be interpreted as relative paths to root */
    cwd = m->root;
    dir++;
  }
  bool done = false;
  while (!done) {
    /* FIXME: the following is broke under symlinks.  I am leaving
       symlink implementation for later. */
    if (cwd->type != is_directory) return 0;
    char *tmp = malloc(strlen(dir)+4);
    int i;
    for (i=0;dir[i] && dir[i] != '/';i++) {
      tmp[i] = dir[i];
    }
    if (!dir[i]) done = true;
    tmp[i] = 0;
    if (!strcmp(tmp, "..")) {
      /* handle .. case as parent directory */
      free(tmp);
      dir += i+1;
      cwd = cwd->parent;
      continue;
    } else if (tmp[0] == 0 || !strcmp(tmp, ".")) {
      /* handle case of two slashes in a row or a . */
      free(tmp);
      dir += i+1;
      continue;
    }
    struct inode *child = (struct inode *)lookup_in_hash(&cwd->c.children, tmp);
    if (!child) {
      char *path = model_realpath(cwd);
      path = realloc(path, strlen(path) + 1 + strlen(tmp)+1);
      strcat(path, "/");
      strcat(path, tmp);
      struct stat st;
      if (!lstat(path, &st) && S_ISLNK(st.st_mode)) {
        child = alloc_symlink(m, cwd, tmp, 0, st.st_size);
        if (child) {
          free(tmp);
          free(path);
          child->is_read = true; /* we read any symlinks that we dereference */
          if (!done) {
            char *newpath = malloc(strlen(child->c.readlink) + 1 + strlen(dir) + 1);
            sprintf(newpath, "%s/%s", child->c.readlink, dir + i + 1);
            struct inode *in = interpret_path_as_directory(m, cwd, newpath);
            free(newpath);
            return in;
          } else {
            return interpret_path_as_directory(m, cwd, child->c.readlink);
          }
        }
      }
      free(path);
      child = alloc_directory(cwd, tmp);
    }
    free(tmp);
    dir += i+1;

    if (child->type == is_symlink) {
      child->is_read = true; /* we read any symlinks that we dereference */
      if (done) {
        return interpret_path_as_directory(m, cwd, child->c.readlink);
      } else {
        char *newpath = malloc(strlen(child->c.readlink) + 1 + strlen(dir) + 1);
        sprintf(newpath, "%s/%s", child->c.readlink, dir);
        struct inode *in = interpret_path_as_directory(m, cwd, newpath);
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

struct inode *model_cwd(struct posixmodel *m, pid_t pid) {
  return lookup_fd(m, pid, CWD);
}

int model_chdir(struct posixmodel *m, struct inode *cwd, const char *dir, pid_t pid) {
  struct inode *thisdir = interpret_path_as_directory(m, cwd, dir);
  if (thisdir) {
    create_fd(m, pid, CWD, thisdir);
    return 0;
  }
  return -1;
}

void model_newthread(struct posixmodel *m, pid_t parent, pid_t child) {
  // this sets the two threads to have the same set of fds
  create_fd(m, child, parent, 0);
}

char *split_at_base(char *path) {
  int len = strlen(path);
  int slashpos = -1;
  for (int i=len-1;i>=0;i--) {
    if (path[i] == '/') {
      slashpos = i;
      break;
    }
  }
  if (slashpos < 0) return 0;
  path[slashpos] = 0;
  return path + slashpos + 1;
}

struct inode *model_lstat(struct posixmodel *m, struct inode *cwd, const char *path0) {
  char *dirpath = strdup(path0);
  const char *basepath = split_at_base(dirpath);
  if (!basepath) {
    free(dirpath);
    dirpath = strdup(".");
    basepath = path0;
  }
  struct inode *dir;
  if (!dirpath[0]) {
    dir = m->root;
  } else {
    dir = interpret_path_as_directory(m, cwd, dirpath);
  }
  if (!dir) {
    free(dirpath);
    return 0;
  }
  struct inode *child = 0;
  if (!strcmp(basepath, "") || !strcmp(basepath, ".")) {
    child = dir;
  } else if (!strcmp(basepath, "..")) {
    child = dir->parent;
  } else {
    child = (struct inode *)lookup_in_hash(&dir->c.children, basepath);
    if (!child) {
      struct stat st;
      char *path;
      if (path0[0] == '/') {
        path = (char *)path0;
      } else {
        path = model_realpath(cwd);
        path = realloc(path, strlen(path)+1+strlen(path0)+4);
        strcat(path, "/");
        strcat(path, path0);
      }
      if (!lstat(path, &st)) {
        if (S_ISLNK(st.st_mode)) {
          child = alloc_symlink(m, dir, basepath, 0, 0);
        } else if (S_ISDIR(st.st_mode)) {
          child = alloc_directory(dir, basepath);
        } else if (S_ISREG(st.st_mode)) {
          child = alloc_file(dir, basepath);
        } else {
          free(dirpath);
          if (path != path0) free(path);
          return 0;
        }
      }
      if (path != path0) free(path);
    }
  }
  free(dirpath);
  return child;
}

struct inode *model_creat(struct posixmodel *m, struct inode *cwd, const char *path0) {
  {
    struct inode *i = model_lstat(m, cwd, path0);
    if (i && i->type == is_file) {
      i->is_written = true;
      return i;
    }
  }
  char *dirpath = strdup(path0);
  const char *basepath = split_at_base(dirpath);
  if (!basepath) {
    free(dirpath);
    dirpath = strdup(".");
    basepath = path0;
  }
  struct inode *dir;
  if (!dirpath[0]) {
    dir = m->root;
  } else {
    dir = interpret_path_as_directory(m, cwd, dirpath);
  }
  if (!dir) {
    free(dirpath);
    return 0;
  }
  struct inode *f = alloc_file(dir, basepath);
  if (f) f->is_written = true;
  free(dirpath);
  return f;
}

struct inode *model_stat(struct posixmodel *m, struct inode *cwd,
                         const char *path) {
  struct inode *i = model_lstat(m, cwd, path);
  if (i && i->type == is_symlink) {
    i->is_read = true;
    return model_stat(m, i->parent, i->c.readlink);
  }
  return i;
}

void free_inode(struct inode *i) {
  if (i->type == is_symlink) {
    free(i->c.readlink);
  } else if (i->type == is_directory) {
    struct inode *ch=(struct inode *)i->c.children.first;
    while (ch) {
      struct inode *tofree = ch;
      ch = (struct inode *)ch->e.next;
      free_inode(tofree);
    }
    free_hash_table(&i->c.children);
  }
  free(i);
}

void model_unlink(struct posixmodel *m, struct inode *cwd, const char *path) {
  struct inode *i = model_lstat(m, cwd, path);
  if (i && i->parent) {
    remove_from_hash(&i->parent->c.children, (struct hash_entry *)i);
    free_inode(i);
  }
}

void model_rename(struct posixmodel *m, struct inode *cwd,
                  const char *from, const char *to) {
  struct inode *i = model_lstat(m, cwd, from);
  remove_from_hash(&i->parent->c.children, (struct hash_entry *)i);

  char *dirpath = strdup(to);
  const char *basepath = split_at_base(dirpath);
  if (!basepath) {
    free(dirpath);
    dirpath = strdup(".");
    basepath = to;
  }
  struct inode *dir = interpret_path_as_directory(m, cwd, dirpath);
  if (!dir) {
    free(dirpath);
    return;
  }
  struct inode *previous = (struct inode *)lookup_in_hash(&dir->c.children, basepath);
  if (previous) {
    remove_from_hash(&dir->c.children, &previous->e);
  }
  i->parent = dir;
  i = realloc(i, sizeof(struct inode) + strlen(basepath) + 1);
  strcpy(i->name, basepath);
  add_to_hash(&dir->c.children, &i->e);
  free(dirpath);
}

int model_mkdir(struct posixmodel *m, struct inode *cwd, const char *dir) {
  struct inode *in = model_lstat(m, cwd, dir);
  if (in) {
    if (in->type == is_directory) return 0;
    return -1;
  }
  struct inode *thisdir = interpret_path_as_directory(m, cwd, dir);
  if (!thisdir) return -1;
  return 0; /* We don't track writes to directories directly */
}

int model_opendir(struct posixmodel *m, struct inode *cwd, const char *dir, pid_t pid, int fd) {
  struct inode *thisdir = interpret_path_as_directory(m, cwd, dir);
  if (thisdir) {
    create_fd(m, pid, fd, thisdir);
    return 0;
  }
  return -1;
}

int model_readdir(struct posixmodel *m, pid_t pid, int fd) {
  struct inode *thisdir = lookup_fd(m, pid, fd);
  if (!thisdir) return -1;
  thisdir->is_read = true;
  return 0;
}

static void inode_output(struct inode *i,
                         hashset *read_from_directories,
                         hashset *read_from_files,
                         hashset *written_to_files) {
  if (i->type == is_directory) {
    if (i->is_read) {
      char *iname = model_realpath(i);
      /* printf("l: %s\n", iname); */
      insert_to_hashset(read_from_directories, iname);
      free(iname);
    }
    for (struct hash_entry *e = i->c.children.first; e; e = e->next) {
      inode_output((struct inode *)e,
                   read_from_directories, read_from_files, written_to_files);
    }
    return;
  }
  if (i->is_written) {
    char *iname = model_realpath(i);
    /* printf("w: %s\n", iname); */
    insert_to_hashset(written_to_files, iname);
    free(iname);
  } else if (i->is_read) {
    char *iname = model_realpath(i);
    /* printf("r: %s\n", iname); */
    insert_to_hashset(read_from_files, iname);
    free(iname);
  }
}

void model_output(struct posixmodel *m,
                  hashset *read_from_directories,
                  hashset *read_from_files,
                  hashset *written_to_files) {
  inode_output(m->root, read_from_directories, read_from_files, written_to_files);
}
