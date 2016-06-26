#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <ftw.h>


#include <unistd.h>
#include <sys/stat.h>

#include "fac.h"
#include "errors.h"

void create_directories(const char *dir) {
  create_parent_directories(dir);
#ifdef _WIN32
  mkdir(dir);
#else
  mkdir(dir, 0777);
#endif
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

void cp_to_dir(const char *fname, const char *dir) {
  FILE *in = fopen(fname, "r");
  if (!in) error(1,errno, "Unable to read file: %s", fname);
  int outlen = strlen(fname) + 1 + strlen(dir) + 1;
  char *outname = malloc(outlen);
  snprintf(outname, outlen, "%s/%s", dir, fname);
  create_parent_directories(outname);
  FILE *out = fopen(outname, "w");
  if (!out) error(1,errno, "Unable to create file: %s", outname);
  static const int bufsize = 4096;
  char *buffer = malloc(bufsize);
  int size_to_write = 1;
  do {
    size_to_write = fread(buffer, 1, bufsize, in);
    fwrite(buffer, 1, size_to_write, out);
  } while (size_to_write);
  free(buffer);
  fclose(in);
  fclose(out);
}
