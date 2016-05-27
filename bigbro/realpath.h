/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2002,2004,2005,2006,2008 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*

  This file is modified from canonical.c from the GNU C library.  The
  copyright header for that file is above.  The realpath function is
  modified here to:

    (a) Keep track of all symlinks that have been deferencenced.

    (b) Enable us to use "realpath" to determine the path *to* a
        symlink that is accessed with lstat or readlink.

  Copyright (C) 2015 David Roundy

  Changes under the same lesser GPL as the original, unlike the rest
  of libbigbro, which is under the GPL, version 2 or later.

  */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>

#include "hashset.h"

typedef struct {
  hashset read, readdir, written, mkdir;
} rw_status;

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated path
   separators ('/') or symlinks.  All path components must exist.  If
   RESOLVED is null, the result is malloc'd; otherwise, if the
   canonical name is PATH_MAX chars or more, returns null with `errno'
   set to ENAMETOOLONG; if the name fits in fewer than PATH_MAX chars,
   returns the name in RESOLVED.  If the name cannot be resolved and
   RESOLVED is non-NULL, it contains the path of the first component
   that cannot be resolved.  If the path can be resolved, RESOLVED
   holds the same value as the value returned.  */

enum last_symlink_handling {
  look_for_file_or_directory, look_for_symlink
};

static inline char *flexible_realpath(const char *name, char *resolved,
                                      rw_status *h,
                                      enum last_symlink_handling lasth,
                                      bool failure_is_okay) {
  char *rpath; // rpath is where we hold the path as we have
               // determined it so far
  char *dest; // dest is the location for the next portion of the path
              // (the end of rpath, as it were)
  char *extra_buf = NULL, *buf = NULL;
  const char *start, *end, *rpath_limit;
  long int path_max;
  int num_links = 0;

  if (name == NULL) {
    /* As per Single Unix Specification V2 we must return an error if
       either parameter is a null pointer.  We extend this to allow
       the RESOLVED parameter to be NULL in case the we are expected to
       allocate the room for the return value.  */
    errno = (EINVAL);
    return NULL;
  }

  if (name[0] == '\0') {
    /* As per Single Unix Specification V2 we must return an error if
       the name argument points to an empty string.  */
    errno = (ENOENT);
    printf("ENOENT\n");
    return NULL;
  }

  path_max = PATH_MAX;

  if (resolved == NULL) {
    rpath = malloc(path_max);
    if (rpath == NULL) {
      printf("malloc failed\n");
      return NULL;
    }
  } else {
    rpath = resolved;
  }
  rpath_limit = rpath + path_max;

  if (name[0] != '/') {
    /* only handle absolute paths! */
    rpath[0] = '\0';
    printf("not absolute\n");
    goto error;
    /* if (!getcwd(rpath, path_max)) { */
    /*   rpath[0] = '\0'; */
    /*   goto error; */
    /* } */
    /* dest = rawmemchr(rpath, '\0'); */
  } else {
    rpath[0] = '/';
    dest = rpath + 1;
  }

  for (start = end = name; *start; start = end) {
    struct stat st;
    int n;

    /* Skip sequence of multiple path-separators.  */
    while (*start == '/') ++start;

    /* Find end of path component.  */
    for (end = start; *end && *end != '/'; ++end) {
      /* Nothing.  */;
    }

    if (end - start == 0) {
      break;
    } else if (end - start == 1 && start[0] == '.') {
      /* nothing */;
    } else if (end - start == 2 && start[0] == '.' && start[1] == '.') {
      /* Back up to previous component, ignore if at root already.  */
      if (dest > rpath + 1)
        while ((--dest)[-1] != '/');
    } else {
      size_t new_size;

      if (dest[-1] != '/') *dest++ = '/';

      if (dest + (end - start) >= rpath_limit) {
        ptrdiff_t dest_offset = dest - rpath;
        char *new_rpath;

        if (resolved) {
          errno = (ENAMETOOLONG);
          if (dest > rpath + 1) dest--;
          *dest = '\0';
          printf("msdgg\n");
          goto error;
        }
        new_size = rpath_limit - rpath;
        if (end - start + 1 > path_max) {
          new_size += end - start + 1;
        } else {
          new_size += path_max;
        }
        new_rpath = (char *) realloc(rpath, new_size);
        if (new_rpath == NULL) {
          printf("remalloc failed here\n");
          goto error;
        }
        rpath = new_rpath;
        rpath_limit = rpath + new_size;

        dest = rpath + dest_offset;
      }

      dest = mempcpy(dest, start, end - start);
      *dest = '\0';

      if ((!*end && lasth == look_for_symlink) || lstat(rpath, &st) < 0) {
        st.st_mode = 0; /* don't treat it as a symlink */
        /* printf("lstat %s failed\n", rpath); */
        /* goto error; */
      }

      if (S_ISLNK(st.st_mode)) {
        if (++num_links > MAXSYMLINKS) {
          errno = (ELOOP);
          printf("loopdsgdsgmalloc failed\n");
          goto error;
        }

        insert_hashset(&h->read, rpath);
        if (!buf) buf = malloc(path_max);
        size_t len;
        n = readlink(rpath, buf, path_max - 1);
        if (n < 0) {
          printf("readlink failed\n");
          goto error;
        }
        buf[n] = '\0';

        if (!extra_buf) extra_buf = malloc(path_max);

        len = strlen (end);
        if ((long int) (n + len) >= path_max) {
          errno = (ENAMETOOLONG);
          printf("malldsdgsdshoc failed\n");
          goto error;
        }

        /* Careful here, end may be a pointer into extra_buf... */
        memmove(&extra_buf[n], end, len + 1);
        name = end = memcpy(extra_buf, buf, n);

        if (buf[0] == '/') {
          dest = rpath + 1;        /* It's an absolute symlink */
        } else {
          /* Back up to previous component, ignore if at root already: */
          if (dest > rpath + 1) {
            while ((--dest)[-1] != '/');
          }
        }
      } else if (!S_ISDIR(st.st_mode) && *end != '\0') {
        errno = (ENOTDIR);
        if (!failure_is_okay)
          fprintf(stderr, "error: %s is not a dir (realpaht of %s, w/ end %s)\n",
                  rpath, name, end);
        goto error;
      }
    }
  }
  if (dest > rpath + 1 && dest[-1] == '/') --dest;
  *dest = '\0';

  assert (resolved == NULL || resolved == rpath);
  if (buf) free(buf);
  if (extra_buf) free(extra_buf);
  return rpath;

error:
  assert (resolved == NULL || resolved == rpath);
  if (buf) free(buf);
  if (extra_buf) free(extra_buf);
  if (resolved == NULL) free(rpath);
  return NULL;
}
