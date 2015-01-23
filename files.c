#define _XOPEN_SOURCE 700

#include "bilge.h"

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

static char *absolute_path(const char *dir, const char *rel) {
  char *myrel = strdup(rel);
  if (*rel == '/') return myrel;
  int len = strlen(myrel);
  for (int i=len-1;i>=0;i--) {
    if (myrel[i] == '/') {
      int lastbit_length = len - i - 1;
      myrel[i] = 0;
      len = strlen(dir)+strlen(myrel)+2;
      char *filename = malloc(len);
      if (snprintf(filename, len, "%s/%s", dir, myrel) >= len)
        error_at_line(1,0, __FILE__, __LINE__, "filename too large!!!");
      char *thepath = realpath(filename, 0);
      if (!thepath)
        error_at_line(1,errno, __FILE__, __LINE__, "filename trouble: %s",
                      filename);
      free(filename);
      free(myrel);
      len = strlen(thepath);
      thepath = realloc(thepath, len+lastbit_length+2);
      if (snprintf(thepath+len, lastbit_length+2, "/%s", rel+strlen(rel)-lastbit_length)
          >= lastbit_length+2)
        error_at_line(1,0, __FILE__, __LINE__, "bug!!!");
      return thepath;
    }
  }
  free(myrel);
  char *thepath = realpath(dir, 0);
  if (!thepath)
    error_at_line(1,errno, __FILE__, __LINE__, "filename trouble");
  int rel_len = strlen(rel);
  int dirlen = strlen(thepath);
  thepath = realloc(thepath, dirlen + rel_len + 2);
  if (snprintf(thepath+dirlen, rel_len+2, "/%s", rel) >= rel_len+2)
    error_at_line(1,0, __FILE__, __LINE__, "bug!!!");
  return thepath;
}

char *done_name(const char *bilgefilename) {
  int bflen = strlen(bilgefilename);
  char *str = malloc(bflen+10);
  sprintf(str, "%s.done", bilgefilename);
  return str;
}

void read_bilge_file(struct all_targets *all, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) error(1, errno, "Unable to open file %s", path);

  char *rel_directory = strdup(path);
  for (int i=strlen(rel_directory)-1;i>=0;i--) {
    if (rel_directory[i] == '/') {
      rel_directory[i] = 0;
      break;
    }
  }
  char *the_directory = realpath(rel_directory, 0);
  if (strlen(rel_directory) == strlen(path)) {
    free(rel_directory);
    free(the_directory);
    rel_directory = ".";
    the_directory = realpath(rel_directory, 0);
  } else {
    free(rel_directory);
  }

  struct rule *therule = 0;
  struct target *thetarget = 0;
  time_t *last_modified_last_file = 0;
  off_t *size_last_file = 0;

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
    case '!':
      create_target(all, one_line+2);
      therule = 0;
      thetarget = 0;
      size_last_file = 0;
      last_modified_last_file = 0;
      break;
    case '|':
      therule = create_rule(all, one_line+2, the_directory);
      therule->bilgefile_path = strdup(path);
      therule->bilgefile_linenum = linenum;
      thetarget = 0;
      size_last_file = 0;
      last_modified_last_file = 0;
      break;
    case '<':
      if (!therule)
        error_at_line(1, 0, path, linenum,
                      "\"<\" input lines must follow a \"|\" command line");
      {
        char *path = absolute_path(the_directory, one_line+2);
        thetarget = create_target(all, path);
        add_explicit_input(therule, thetarget);
        last_modified_last_file = &therule->input_times[therule->num_inputs-1];
        size_last_file = &therule->input_sizes[therule->num_inputs-1];
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
        add_explicit_output(therule, thetarget);
        last_modified_last_file = &therule->output_times[therule->num_outputs-1];
        size_last_file = &therule->output_sizes[therule->num_outputs-1];
        free(path);
      }
      break;
    case 'T':
      if (!last_modified_last_file)
        error_at_line(1, 0, path, linenum,
                      "\"T\" modification-time lines must follow a file specification");
      if (sscanf(one_line+2, "%ld", last_modified_last_file) != 1)
        error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
      break;
    case 'S':
      if (!size_last_file)
        error_at_line(1, 0, path, linenum,
                      "\"S\" file-size lines must follow a file specification");
      if (sscanf(one_line+2, "%ld", size_last_file) != 1)
        error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
      break;
    case 'B':
      if (!therule)
        error_at_line(1, 0, path, linenum,
                      "\"B\" build-time lines must follow a \"|\" command line");
      if (sscanf(one_line+2, "%lg", &therule->build_time) != 1)
        error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
      break;
    }
  }
  if (!feof(f))
    error(1, errno, "Error reading file %s", path);
  fclose(f);

  char *donename = done_name(path);
  f = fopen(donename, "r");
  if (f) {
    linenum = 0;
    while (getline(&one_line, &buffer_length, f) >= 0) {
      linenum++;
      int line_length = strlen(one_line);
      if (line_length > 0 && one_line[line_length-1] == '\n')
        one_line[line_length-- -1] = 0; /* trim newline */

      if (line_length < 2) continue;
      if (one_line[0] == '#') continue; /* it is a comment! */

      if (one_line[1] != ' ')
        error_at_line(1, 0, donename, linenum,
                    "Second character of line should be a space");
      switch (one_line[0]) {
      case '|':
        therule = lookup_rule(all, one_line+2, the_directory);
        //printf(":: %s\n", therule->command);
        thetarget = 0;
        size_last_file = 0;
        last_modified_last_file = 0;
        break;
      case '<':
        if (therule) {
          char *path = absolute_path(the_directory, one_line+2);
          thetarget = create_target(all, path);
          /* The following check is to deal with the case where a
             given file might actually be an output of a command, but
             was only read when being built, by a program that tries
             to be friendly to make by avoiding touching a file that
             would end up being identical when built. */
          if (thetarget->rule == therule) {
            /* It was actually an output, so let's trust the user and
               treat it as such. */
            add_output(therule, thetarget);
            for (int i=0; i<therule->num_outputs; i++) {
              if (thetarget == therule->outputs[i]) {
                last_modified_last_file = &therule->output_times[i];
                size_last_file = &therule->output_sizes[i];
                break;
              }
            }
          } else {
            //printf("  I see #%d %s\n", therule->num_inputs-1, path);
            add_input(therule, thetarget);
            for (int i=0;i<therule->num_inputs;i++) {
              if (thetarget == therule->inputs[i]) {
                last_modified_last_file = &therule->input_times[i];
                size_last_file = &therule->input_sizes[i];
                break;
              }
            }
          }
          free(path);
        }
        break;
      case '>':
        if (therule) {
          char *path = absolute_path(the_directory, one_line+2);
          thetarget = create_target(all, path);
          thetarget->rule = therule;
          add_output(therule, thetarget);
          for (int i=0; i<therule->num_outputs; i++) {
            if (thetarget == therule->outputs[i]) {
              last_modified_last_file = &therule->output_times[i];
              size_last_file = &therule->output_sizes[i];
              break;
            }
          }
          free(path);
        }
        break;
      case 'T':
        if (last_modified_last_file) {
          if (sscanf(one_line+2, "%ld", last_modified_last_file) != 1)
            error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
        }
        break;
      case 'S':
        if (size_last_file) {
          if (sscanf(one_line+2, "%ld", size_last_file) != 1)
            error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
        }
        break;
      case 'B':
        if (therule) {
          if (sscanf(one_line+2, "%lg", &therule->build_time) != 1)
            error_at_line(1, 0, path, linenum, "Error parsing %s", one_line);
        }
        break;
      }
    }
    if (!feof(f))
      error(1, errno, "Error reading file %s", donename);
    fclose(f);
  }

  free(donename);
  free(one_line);
  free(the_directory);
}

void fprint_bilgefile(FILE *f, struct all_targets *tt, const char *bpath) {
  for (struct rule *r = (struct rule *)tt->r.first; r; r = (struct rule *)r->e.next) {
    if (!strcmp(r->bilgefile_path, bpath)) {
      fprintf(f, "| %s\n", r->command);
      if (r->working_directory &&
          strcmp(r->working_directory, ".")) {
        fprintf(f, ". %s\n", r->working_directory);
      }
      if (r->build_time) {
        fprintf(f, "B %g\n", r->build_time);
      }
      for (int i=0; i<r->num_outputs; i++) {
        fprintf(f, "> %s\n", r->outputs[i]->path);
        if (r->outputs[i]->last_modified) {
          fprintf(f, "T %ld\n", r->outputs[i]->last_modified);
          fprintf(f, "S %ld\n", r->outputs[i]->size);
        }
      }
      for (int i=0; i<r->num_inputs; i++) {
        fprintf(f, "< %s\n", r->inputs[i]->path);
        if (r->inputs[i]->last_modified) {
          fprintf(f, "T %ld\n", r->inputs[i]->last_modified);
          fprintf(f, "S %ld\n", r->inputs[i]->size);
        }
      }
      fprintf(f, "\n");
    }
  }
}

void print_bilge_file(struct all_targets *tt) {
  for (struct rule *r = (struct rule *)tt->r.first; r; r = (struct rule *)r->e.next) {
    r->status = unknown;
  }
  for (struct rule *r = (struct rule *)tt->r.first; r; r = (struct rule *)r->e.next) {
    r->status = unknown;
    if (r->status == unknown) {
      printf("| %s\n", r->command);
      if (r->working_directory &&
          strcmp(r->working_directory, ".")) {
        printf(". %s\n", r->working_directory);
      }
      for (int i=0; i<r->num_inputs; i++) {
        printf("< %s\n", r->inputs[i]->path);
        if (r->inputs[i]->last_modified) {
          printf("T %ld\n", r->inputs[i]->last_modified);
          printf("S %ld\n", r->inputs[i]->size);
        }
      }
      for (int i=0; i<r->num_outputs; i++) {
        printf("> %s\n", r->outputs[i]->path);
        if (r->outputs[i]->last_modified) {
          printf("T %ld\n", r->outputs[i]->last_modified);
          printf("S %ld\n", r->outputs[i]->size);
        }
      }
      printf("\n");
      r->status = built;
    }
  }
}
