#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fac.h"

void create_directories(const char *dir) {
  create_parent_directories(dir);
  mkdir(dir, 0777);
}

void create_parent_directories(const char *fname) {
  char *dirname = malloc(strlen(fname)+1);
  strcpy(dirname, fname);
  if (strrchr(dirname,  '/')) {
    *strrchr(dirname,  '/') = 0; // remove last segment of dirname
    create_directories(dirname);
  }
  free(dirname);
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    return remove(fpath);
}

int rm_recursive(const char *path) {
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
