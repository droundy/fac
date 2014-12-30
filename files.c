#define _XOPEN_SOURCE 700

#include "bilge.h"

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

static char *mycopy(const char *str) {
  char *out = malloc(strlen(str)+1);
  strcpy(out, str);
  return out;
}

static char *absolute_path(const char *dir, const char *rel) {
  int len = strlen(dir)+strlen(rel)+2;
  char *filename = malloc(len);
  if (snprintf(filename, len, "%s/%s", dir, rel) >= len)
    error_at_line(1,0, __FILE__, __LINE__, "filename too large!!!");
  char *thepath = realpath(filename, 0);
  free(filename);
  return thepath;
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
  free(rel_directory);

  struct rule *therule = 0;
  struct target *thetarget = 0;

  int linenum = 0;
  char *one_line = 0;
  size_t buffer_length = 0;
  while (getline(&one_line, &buffer_length, f) >= 0) {
    linenum++;
    int line_length = strlen(one_line);
    if (line_length > 0 && one_line[line_length-1] == '\n')
      one_line[line_length-- -1] = 0; /* trim newline */

    if (line_length < 2) continue;
    if (one_line[0] == '#') continue; /* it is a comment! */

    if (one_line[1] != ' ')
      error_at_line(1, 0, path, linenum,
                    "Second character of line should be a space");
    switch (one_line[0]) {
    case '|':
      therule = create_rule(one_line+2, the_directory);
      therule->bilgefile_path = mycopy(path);
      therule->bilgefile_linenum = linenum;
      thetarget = 0;
      break;
    case '<':
      if (!therule)
        error_at_line(1, 0, path, linenum,
                      "\"<\" input lines must follow a \"|\" command line");
      {
        char *path = absolute_path(the_directory, one_line+2);
        thetarget = create_target(all, path);
        add_input(therule, thetarget);
        free(path);
      }
      break;
    case '>':
      if (!therule)
        error_at_line(1, 0, path, linenum,
                      "\">\" output lines must follow a \"|\" command line");
      {
        char *path = absolute_path(the_directory, one_line+2);
        thetarget = create_target(all, path);
        thetarget->rule = therule;
        add_output(therule, thetarget);
        free(path);
      }
      break;
    case 'T':
      if (!thetarget)
        error_at_line(1, 0, path, linenum,
                      "\"T\" modification-time lines must follow a file specification");
      if (sscanf(one_line+2, "%ld", &thetarget->last_modified) != 1)
        error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
      break;
    case 'S':
      if (!thetarget)
        error_at_line(1, 0, path, linenum,
                      "\"S\" file-size lines must follow a file specification");
      if (sscanf(one_line+2, "%ld", &thetarget->size) != 1)
        error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
      break;
    }
  }
  free(one_line);
  free(the_directory);
  if (!feof(f))
    error(1, errno, "Error reading file %s", path);
  fclose(f);
}

void print_bilge_file(struct all_targets *tt) {
  struct all_targets *t = tt;
  while (t) {
    if (t->t->rule) t->t->rule->status = unknown;
    t = t->next;
  }
  while (tt) {
    if (tt->t->rule && tt->t->rule->status == unknown) {
      printf("| %s\n", tt->t->rule->command);
      if (tt->t->rule->working_directory &&
          strcmp(tt->t->rule->working_directory, ".")) {
        printf(". %s\n", tt->t->rule->working_directory);
      }
      for (int i=0; i<tt->t->rule->num_inputs; i++) {
        printf("< %s\n", tt->t->rule->inputs[i]->path);
        if (tt->t->rule->inputs[i]->last_modified) {
          printf("T %ld\n", tt->t->rule->inputs[i]->last_modified);
          printf("S %ld\n", tt->t->rule->inputs[i]->size);
        }
      }
      for (int i=0; i<tt->t->rule->num_outputs; i++) {
        printf("> %s\n", tt->t->rule->outputs[i]->path);
        if (tt->t->rule->outputs[i]->last_modified) {
          printf("T %ld\n", tt->t->rule->outputs[i]->last_modified);
          printf("S %ld\n", tt->t->rule->outputs[i]->size);
        }
      }
      printf("\n");
      tt->t->rule->status = built;
    }
    tt = tt->next;
  }
}
