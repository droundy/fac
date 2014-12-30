#define _XOPEN_SOURCE 700

#include "bilge.h"

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

static char *mycopy(const char *str) {
  char *out = malloc(strlen(str)+1);
  strcpy(out, str);
  return out;
}

void read_bilge_file(struct all_targets **all, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) error(1, errno, "Unable to open file %s", path);

  char *rel_directory = mycopy(path);
  for (int i=strlen(rel_directory)-1;i>=0;i--) {
    if (rel_directory[i] == '/') {
      rel_directory[i] = 0;
      break;
    }
  }
  if (strlen(rel_directory) == strlen(path)) {
    free(rel_directory);
    rel_directory = ".";
  }
  char *the_directory = realpath(rel_directory, 0);

  struct rule *therule = 0;

  char *one_line = 0;
  size_t buffer_length = 0;
  while (getline(&one_line, &buffer_length, f) >= 0) {
    int line_length = strlen(one_line);
    if (line_length > 0 && one_line[line_length-1] == '\n')
      one_line[line_length-- -1] = 0; /* trim newline */

    if (line_length == 0) continue;

    switch (one_line[0]) {
    case '|':
      therule = create_rule(one_line+1, the_directory);
      break;
    case '<':
        assert(therule);
        {
          struct target *t = lookup_target(*all, one_line+1);
          if (!t) {
            t = create_target(one_line+1);
            insert_target(all, t);
          }
          add_input(therule, t);
        }
        break;
    case '>':
        assert(therule);
        {
          struct target *t = lookup_target(*all, one_line+1);
          if (!t) {
            t = create_target(one_line+1);
            insert_target(all, t);
          }
          t->rule = therule;
          add_output(therule, t);
        }
        break;
    }
  }
  free(one_line);
  if (!feof(f))
    error(1, errno, "Error reading file %s", path);
}

void print_bilge_file(struct all_targets *tt) {
  struct all_targets *t = tt;
  while (t) {
    if (t->t->rule) t->t->rule->status = unknown;
    t = t->next;
  }
  while (tt) {
    if (tt->t->rule && tt->t->rule->status == unknown) {
      printf("|%s\n", tt->t->rule->command);
      for (int i=0; i<tt->t->rule->num_inputs; i++) {
        printf("<%s\n", tt->t->rule->inputs[i]->path);
      }
      for (int i=0; i<tt->t->rule->num_outputs; i++) {
        printf(">%s\n", tt->t->rule->outputs[i]->path);
      }
      printf("\n");
      tt->t->rule->status = built;
    }
    tt = tt->next;
  }
}
