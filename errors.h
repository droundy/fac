#ifndef ERRORS_H
#define ERRORS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern int verbose; /* true if user requests verbose output */

static inline void verbose_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (verbose) vfprintf(stdout, format, args);
  va_end(args);
}

static inline void error_at_line(int retval, int my_errno, const char *fname,
                                 int linenum, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "error: %s:%d: ", fname, linenum);
  vfprintf(stderr, format, args);
  if (my_errno) fprintf(stderr, "\n  %s\n", strerror(my_errno));
  va_end(args);
  exit(retval);
}

static inline void error(int retval, int my_errno, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "error: ");
  vfprintf(stderr, format, args);
  if (my_errno) fprintf(stderr, "\n  %s\n", strerror(my_errno));
  va_end(args);
  exit(retval);
}

#endif
