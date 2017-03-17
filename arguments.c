#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arguments.h"

enum argument_type {
  NO_ARG,
  INT_ARG,
  STRING_ARG,
  STRINGLIST_ARG
};

struct a {
  const char *long_name;
  char short_name;
  const char *description;
  const char *fieldname;
  enum argument_type type;
  const char **string_location;
  const char ***stringlist_location;
  int *int_location;
  bool *bool_location;
};

static int num_arguments = 0;
static struct a *args = NULL;

static struct a * add_argument(void) {
  num_arguments++;
  args = (struct a *)realloc(args, num_arguments*sizeof(struct a));
  return &args[num_arguments - 1];
}

void string_argument(const char *long_name, char short_name, char **output,
                     const char *description, const char *outname) {
  struct a *a = add_argument();
  a->long_name = long_name;
  a->short_name = short_name;
  a->string_location = (const char **)output;
  a->type = STRING_ARG;
  a->description = description;
  a->fieldname = outname;
}

void string_argument_list(const char *long_name, char short_name, const char ***output,
                          const char *description, const char *outname) {
  struct a *a = add_argument();
  a->long_name = long_name;
  a->short_name = short_name;
  a->stringlist_location = output;
  a->type = STRINGLIST_ARG;
  a->description = description;
  a->fieldname = outname;
  if (!*a->stringlist_location) {
    *a->stringlist_location = malloc(sizeof(char **));
    **a->stringlist_location = NULL;
  }
}

void int_argument(const char *long_name, char short_name, int *output,
                  const char *description, const char *outname) {
  struct a *a = add_argument();
  a->long_name = long_name;
  a->short_name = short_name;
  a->int_location = output;
  a->type = INT_ARG;
  a->description = description;
  a->fieldname = outname;
}

void no_argument(const char *long_name, char short_name, bool *output,
                 const char *description) {
  struct a *a = add_argument();
  a->long_name = long_name;
  a->short_name = short_name;
  a->type = NO_ARG;
  a->bool_location = output;
  a->description = description;
  a->fieldname = NULL;
}

static bool read_int(const char *str, int *val) {
  char *end = NULL;
  long int v = strtol(str, &end, 0);
  if (end != str + strlen(str)) {
    return false;
  }
  if ((long)(int)v != v) {
    // not in range
    return false;
  }
  *val = v;
  return true;
}

static void handle_arg(struct a *the_a, const char *the_arg) {
  if (the_a->type == INT_ARG) {
    if (!read_int(the_arg, the_a->int_location)) {
      fprintf(stderr, "invalid integer argument --%s '%s'\n", the_a->long_name, the_arg);
      exit(1);
    }
  } else if (the_a->type == STRING_ARG) {
    *(the_a->string_location) = the_arg;
  } else if (the_a->type == STRINGLIST_ARG) {
    int num_strings;
    for (num_strings=0; (*the_a->stringlist_location)[num_strings]; num_strings++) {
      // counting how many strings we have already.
    }
    *the_a->stringlist_location = realloc(*(the_a->stringlist_location),
                                          (num_strings+2)*sizeof(char **));
    (*the_a->stringlist_location)[num_strings+1] = NULL;
    (*the_a->stringlist_location)[num_strings] = the_arg;
  } else if (the_a->type == NO_ARG) {
    *(the_a->bool_location) = true;
  }
}

const char **parse_arguments_return_extras(const char **argv) {
  argv++;
  int num_output = 0;
  const char **output = NULL;
  for (const char **here = argv; *here; here++) {
    if (strcmp(*here, "--help") == 0) {
      bool help_argument;
      no_argument("help", 'h', &help_argument,
                  "show this help message");
      printf("Usage: fac [OPTIONS] [TARGETS]\n");
      for (int j=0; j<num_arguments; j++) {
        int chars_written = 0;
        if (args[j].short_name) {
          chars_written += printf(" -%c,", args[j].short_name);
        }
        while (chars_written < 4) {
          printf(" ");
          chars_written++;
        }
        if (args[j].long_name) {
          chars_written += printf(" --%s", args[j].long_name);
        }
        if (args[j].fieldname) {
          chars_written += printf(" %s", args[j].fieldname);
        }
        while (chars_written < 32) {
          printf(" ");
          chars_written++;
        }
        printf(" %s\n", args[j].description);
      }
      exit(0);
    } else if (strncmp(*here, "--", 2) == 0) {
      const char *name = *here + 2;
      const char *the_arg = NULL;
      struct a *the_a = NULL;
      for (int j=0; j<num_arguments; j++) {
        if (strcmp(args[j].long_name, name) == 0) {
          the_a = &args[j];
          if (args[j].type != NO_ARG) {
            here++;
            the_arg = *here;
          }
          break;
        }
        int arglen = strlen(args[j].long_name);
        if (args[j].type != NO_ARG &&
            strncmp(args[j].long_name, name, arglen) == 0 &&
            name[arglen] == '=') {
          the_arg = name + arglen + 1;
          the_a = &args[j];
          break;
        }
      }
      if (!the_a) {
        fprintf(stderr, "invalid argument --%s\n", name);
        exit(1);
      }
      handle_arg(the_a, the_arg);
    } else if (strncmp(*here, "-", 1) == 0) {
      for (int i=1; i<strlen(*here); i++) {
        char c = (*here)[i];
        struct a *the_a = NULL;
        const char *the_arg = NULL;
        for (int j=0; j<num_arguments; j++) {
          if (args[j].short_name == c) {
            the_a = &args[j];
            break;
          }
        }
        if (!the_a) {
          fprintf(stderr, "invalid argument -%c\n", c);
          exit(1);
        }
        if (the_a->type != NO_ARG) {
          if (i < strlen(*here)-1) {
            the_arg = *here + i+1;
            i = strlen(*here);
          }
        }
        if (!the_arg && the_a->type != NO_ARG) {
          here++;
          the_arg = *here;
        }
        handle_arg(the_a, the_arg);
        if (the_arg) {
          break;
        }
      }
    } else {
      num_output++;
      output = (const char **)realloc(output, num_output*sizeof(char *));
      output[num_output-1] = *here;
    }
  }
  num_output++;
  output = (const char **)realloc(output, num_output*sizeof(char *));
  output[num_output-1] = NULL;
  return output;
}

