#define _XOPEN_SOURCE 700

#include "errors.h"
#include "fac.h"
#include "environ.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#ifdef _WIN32
/* fixme: the following is a very broken version of realpath for windows! */
char *realpath(const char *p, int i) {
  char *r = malloc(strlen(p));
  strcpy(r, p);
  return r;
}
#else
#include <unistd.h>
#endif


#if defined(_WIN32) || defined(__APPLE__)
char *getline(char **lineptr, size_t *n, FILE *stream) {
  int toread = *n;
  char *p = *lineptr;
  while (fgets(p, toread, stream), strlen(p) == toread-1) {
    toread = *n;
    *n *= 2;
    *lineptr = realloc(*lineptr, *n);
    p = *lineptr + toread - 1;
  }
  return *lineptr;
}
#endif

char *absolute_path(const char *dir, const char *rel) {
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
      if (!thepath) {
        if (errno != ENOENT) {
          fprintf(stderr, "Difficulty disambiguating %s: %s\n",
                  filename, strerror(errno));
        }
        thepath = filename;
      } else {
        free(filename);
      }
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
#ifdef _WIN32
    error_at_line(1,0, __FILE__, __LINE__, "filename trouble");
#else
    error_at_line(1,errno, __FILE__, __LINE__, "filename trouble");
#endif
  int rel_len = strlen(rel);
  int dirlen = strlen(thepath);
  thepath = realloc(thepath, dirlen + rel_len + 2);
  if (snprintf(thepath+dirlen, rel_len+2, "/%s", rel) >= rel_len+2)
    error_at_line(1,0, __FILE__, __LINE__, "bug!!!");
  return thepath;
}

const char *relative_path(const char *myroot, const char *path) {
  int len = strlen(myroot);
  int pathlen = strlen(path);
  if (pathlen > len && path[len] == '/' && !memcmp(path, myroot, len)) {
    return path + len + 1;
  }
  return path;
}


char *done_name(const char *facfilename) {
  int bflen = strlen(facfilename);
  char *str = malloc(bflen+5);
  sprintf(str, "%s.tum", facfilename);
  return str;
}

void read_fac_file(struct all_targets *all, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "error: unable to open file %s\n  %s\n",
            path, strerror(errno));
    exit(1);
  }

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
  struct hashstat *stat_last_file = 0;

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
      error_at_line(1, 0, pretty_path(path), linenum,
                    "Second character of line should be a space");
    switch (one_line[0]) {
    case '?':
    case '|':
      {
        struct rule *existing = lookup_rule(all, one_line+2, the_directory);
        if (existing)
          error_at_line(1, 0, pretty_path(path), linenum,
                        "duplicate rule:  %s\nalso defined in %s:%d\n",
                        one_line+2, existing->facfile_path, existing->facfile_linenum);
        therule = create_rule(all, path, one_line+2, the_directory);
        therule->facfile_linenum = linenum;
        thetarget = 0;
        stat_last_file = 0;
        if (one_line[0] == '?') therule->is_default = false;
      }
      break;
    case 'C':
      if (!therule)
        error_at_line(1, 0, pretty_path(path), linenum,
                      "\"C\" cache lines must follow a \"|\" command line");
      {
        const char *prefix = one_line+2;
        if (strlen(prefix) > 2 && prefix[0] == '~' && prefix[1] == '/') {
          /* It is in the home directory.  We use realpath to handle
             the case where the home directory defined in $HOME
             actually is a symlink to the real home directory. */
          const char *home = realpath(getenv("HOME"), 0);
          if (!home) {
            fprintf(stderr, "ignoring %s directive since $HOME does not exist...\n",
                    one_line);
          } else {
            const int len = strlen(home) + strlen(prefix) + 1;
            char *absolute_prefix = malloc(len);
            strncpy(absolute_prefix, home, len);
            strncat(absolute_prefix, prefix+1, len);
            add_cache_prefix(therule, absolute_prefix);
            free(absolute_prefix);
          }
        } else {
          char *path = absolute_path(the_directory, one_line+2);
          add_cache_prefix(therule, path);
          free(path);
        }
      }
      break;
    case 'c':
      if (!therule)
        error_at_line(1, 0, pretty_path(path), linenum,
                      "\"c\" cache lines must follow a \"|\" command line");
      add_cache_suffix(therule, one_line+2);
      break;
    case '<':
      if (!therule)
        error_at_line(1, 0, pretty_path(path), linenum,
                      "\"<\" input lines must follow a \"|\" command line");
      {
        char *path = absolute_path(the_directory, one_line+2);
        thetarget = create_target(all, path);
        add_explicit_input(therule, thetarget);
        stat_last_file = &therule->input_stats[therule->num_inputs-1];
        free(path);
      }
      break;
    case '>':
      if (!therule)
        error_at_line(1, 0, pretty_path(path), linenum,
                      "\">\" output lines must follow a \"|\" command line");
      {
        if (one_line[2] == '/' || one_line[2] == '~') {
          // this looks like an absolute path, which is wrong.
          error_at_line(1, 0, pretty_path(path), linenum,
                        "\">\" cannot be followed by an absolute path (%s)\n",
                        one_line+2);
        }
        char *filepath = absolute_path(the_directory, one_line+2);
        thetarget = create_target(all, filepath);
        if (thetarget->rule && therule != thetarget->rule) {
          error_at_line(1, 0, pretty_path(path), linenum,
                        "two rules to create the same file: %s\n| %s (in %s:%d)\n| %s (in %s:%d)\n",
                        one_line+2,
                        therule->command,
                        pretty_path(path),
                        therule->facfile_linenum,
                        thetarget->rule->command,
                        pretty_path(thetarget->rule->facfile_path),
                        thetarget->rule->facfile_linenum);
        }
        if (therule != thetarget->rule) {
          thetarget->rule = therule;
          add_explicit_output(therule, thetarget);
          stat_last_file = &therule->output_stats[therule->num_outputs-1];
          free(filepath);
        }
      }
      break;
    }
  }
  if (!feof(f)) {
    fprintf(stderr, "error: reading file %s\n  %s", path, strerror(errno));
    exit(1);
  }
  fclose(f);

  char *donename = done_name(path);
  /* am_deleting_output indicates if a rule has been removed, and we
     need to clean up the output of said rule. */
  bool am_deleting_output = false;
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

      /* in case of error in the done file, just ignore the rest of
         it. */
      if (one_line[1] != ' ') break;

      switch (one_line[0]) {
      case '|':
        am_deleting_output = false;
        therule = lookup_rule(all, one_line+2, the_directory);
        if (!therule) am_deleting_output = true;
        //printf(":: %s\n", therule->command);
        thetarget = 0;
        stat_last_file = 0;
        break;
      case '<':
        if (therule) {
          char *path = absolute_path(the_directory, one_line+2);
          if (is_interesting_path(therule, path)) {
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
                  stat_last_file = &therule->output_stats[i];
                  break;
                }
              }
            } else {
              //printf("  I see #%d %s\n", therule->num_inputs-1, path);
              add_input(therule, thetarget);
              for (int i=0;i<therule->num_inputs;i++) {
                if (thetarget == therule->inputs[i]) {
                  stat_last_file = &therule->input_stats[i];
                  break;
                }
              }
            }
          } else {
            // It is cached, so we need to ignore any stats of this file!
            thetarget = 0;
            stat_last_file = 0;
          }
          free(path);
        }
        break;
      case '>':
        if (therule) {
          char *path = absolute_path(the_directory, one_line+2);
          if (is_interesting_path(therule, path)) {
            thetarget = create_target(all, path);
            if (!thetarget->rule && is_facfile(path)) {
              /* We ignore any generated facfiles that were not
                 explicitly requested! */
              thetarget->status = dirty; /* this says to not read this file */
            } else {
              if (!thetarget->rule || thetarget->rule == therule) {
                thetarget->rule = therule;
                add_output(therule, thetarget);
                for (int i=0; i<therule->num_outputs; i++) {
                  if (thetarget == therule->outputs[i]) {
                    stat_last_file = &therule->output_stats[i];
                    break;
                  }
                }
              }
            }
          } else {
            /* It is a cache file, so we need to ignore any stats of
               this file! */
            thetarget = 0;
            stat_last_file = 0;
          }
          free(path);
        } else if (am_deleting_output) {
          char *path = absolute_path(the_directory, one_line+2);
          unlink(path);
          free(path);
        }
        break;
      case 'T':
        if (stat_last_file) {
          /* ignore errors in the done file: */
          stat_last_file->time = strtol(one_line+2, 0, 0);
        }
        break;
      case 'S':
        if (stat_last_file) {
          /* ignore errors in the done file: */
          stat_last_file->size = strtol(one_line+2, 0, 0);
        }
        break;
      case 'H':
        if (stat_last_file) {
          stat_last_file->hash = read_sha1(one_line+2);
        }
        break;
      case 'B':
        if (therule) {
          all->estimated_times[therule->status] -= therule->build_time;
          /* ignore errors in the done file: */
          sscanf(one_line+2, "%lg", &therule->build_time);
          all->estimated_times[therule->status] += therule->build_time;
        }
        break;
      case 'E':
        if (therule) {
          int num = sscanf(one_line+2,
                           "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                           &therule->env.u8[ 0], &therule->env.u8[ 1], &therule->env.u8[ 2],
                           &therule->env.u8[ 3], &therule->env.u8[ 4], &therule->env.u8[ 5],
                           &therule->env.u8[ 6], &therule->env.u8[ 7], &therule->env.u8[ 8],
                           &therule->env.u8[ 9], &therule->env.u8[10], &therule->env.u8[11],
                           &therule->env.u8[12], &therule->env.u8[13], &therule->env.u8[14],
                           &therule->env.u8[15], &therule->env.u8[16], &therule->env.u8[17],
                           &therule->env.u8[18], &therule->env.u8[19]);
          if (num != 20) {
            printf("Error parsing %s got %d\n", one_line, num);
          }
        }
        break;
      }
    }
    fclose(f);
  }

  free(donename);
  free(one_line);
  free(the_directory);
}

void fprint_facfile(FILE *f, struct all_targets *tt, const char *bpath) {
  for (struct rule *r = (struct rule *)tt->r.first; r; r = (struct rule *)r->e.next) {
    if (!strcmp(r->facfile_path, bpath)) {
      fprintf(f, "| %s\n", r->command);
      if (r->status != failed) {
        if (r->build_time) {
          fprintf(f, "B %g\n", r->build_time);
        }
        if (r->env.abc.a || r->env.abc.b || r->env.abc.c) {
          fprintf(f, "E ");
          for (int i=0;i<20;i++) {
            fprintf(f, "%02x", r->env.u8[i]);
          }
          fprintf(f, "\n");
        }
      }
      for (int i=0; i<r->num_outputs; i++) {
        fprintf(f, "> %s\n", relative_path(r->working_directory, r->outputs[i]->path));
        if (r->output_stats[i].time && r->status != failed) {
          fprintf(f, "T %ld\n", (long)r->output_stats[i].time);
          fprintf(f, "S %ld\n", (long)r->output_stats[i].size);
          sha1hash h = r->output_stats[i].hash;
          if (h.abc.a || h.abc.b || h.abc.c) {
            fprintf(f, "H ");
            fprint_sha1(f, h);
            fprintf(f, "\n");
          }
        }
      }
      for (int i=0; i<r->num_inputs; i++) {
        fprintf(f, "< %s\n", relative_path(r->working_directory, r->inputs[i]->path));
        if (r->input_stats[i].time) {
          fprintf(f, "T %ld\n", (long)r->input_stats[i].time);
          fprintf(f, "S %ld\n", (long)r->input_stats[i].size);
          sha1hash h = r->input_stats[i].hash;
          if (h.abc.a || h.abc.b || h.abc.c) {
            fprintf(f, "H ");
            fprint_sha1(f, h);
            fprintf(f, "\n");
          }
        }
      }
      fprintf(f, "\n");
    }
  }
}

static int target_cmp(const void *rr1, const void *rr2) {
  const struct target *r1 = *(struct target **)rr1;
  const struct target *r2 = *(struct target **)rr2;
  return strcmp(r1->path, r2->path);
}

static void sort_target_array(struct target **t, int num) {
  qsort(t, num, sizeof(struct target *), target_cmp);
}

static void fprint_makefile_escape(FILE *f, const char *path) {
  while (*path) {
    switch (*path) {
    case ' ':
    case '$':
    case '\\':
    case '"':
    case '\'':
      fputc('\\', f);
      fputc(*path, f);
      break;
    default:
      fputc(*path, f);
    }
    path++;
  }
}

static sha1hash dot_nodename(const struct target *t) {
  sha1nfo s;
  sha1_init(&s);
  sha1_write(&s, t->path, strlen(t->path));
  return sha1_out(&s);
}

static const int max_inputs_for_dot = 70;

static void fprint_dot_rule(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  int num_repo_inputs = 0;
  for (int i=0;i<r->num_inputs;i++) {
    if (is_in_root(r->inputs[i]->path)) num_repo_inputs++;
  }
  if (num_repo_inputs > max_inputs_for_dot) return;

  /* We sort the inputs and outputs before creating the makefile, so
     as to generate output that is the same on every computer, and on
     every invocation.  This is helpful because often one wants to put
     the generated file into the git repository for the benefit of
     users who have not installed fac. */
  struct target **inps = malloc(r->num_inputs*sizeof(struct target *));
  for (int i=0; i<r->num_inputs; i++) inps[i] = r->inputs[i];
  sort_target_array(inps, r->num_inputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (inps[i]->rule) fprint_dot_rule(f, inps[i]->rule);
  }

  struct target **outs = malloc(r->num_outputs*sizeof(struct target *));
  for (int i=0; i<r->num_outputs; i++) outs[i] = r->outputs[i];
  sort_target_array(outs, r->num_outputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (is_in_root(inps[i]->path)) {
      for (int j=0; j<r->num_outputs; j++) {
        fprintf(f, "   node");
        fprint_sha1(f, dot_nodename(inps[i]));
        fprintf(f, " -> node");
        fprint_sha1(f, dot_nodename(outs[j]));
        fprintf(f, ";\n");
      }
    }
  }
  free(inps);
  free(outs);
}

static void fprint_dot_nodes(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  const int lenroot = strlen(root);
  r->is_printed = true;

  /* We sort the inputs and outputs before creating the makefile, so
     as to generate output that is the same on every computer, and on
     every invocation.  This is helpful because often one wants to put
     the generated file into the git repository for the benefit of
     users who have not installed fac. */
  struct target **inps = malloc(r->num_inputs*sizeof(struct target *));
  for (int i=0; i<r->num_inputs; i++) inps[i] = r->inputs[i];
  sort_target_array(inps, r->num_inputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (inps[i]->rule) fprint_dot_nodes(f, inps[i]->rule);
  }

  bool is_intermediate = false;
  for (int i=0;i<r->num_outputs;i++) {
    if (r->outputs[i]->num_children) is_intermediate = true;
  }
  fprintf(f, "# %s\n", r->command);
  const char *color = "black";
  if (is_intermediate) color = "blue";
  for (int i=0;i<r->num_outputs;i++) {
    if (!r->outputs[i]->is_printed) {
      fprintf(f, "   node");
      fprint_sha1(f, dot_nodename(r->outputs[i]));
      fprintf(f, "[label=\"%s\", color=\"%s\"];\n",
              r->outputs[i]->path + lenroot+1, color);
      r->outputs[i]->is_printed = true;
    }
  }

  int num_repo_inputs = 0;
  for (int i=0;i<r->num_inputs;i++) {
    if (is_in_root(r->inputs[i]->path)) num_repo_inputs++;
  }
  if (num_repo_inputs > max_inputs_for_dot) return;

  for (int i=0;i<r->num_inputs;i++) {
    if (!r->inputs[i]->rule) {
      if (!r->inputs[i]->is_printed && is_in_root(r->inputs[i]->path)) {
        fprintf(f, "   node");
        fprint_sha1(f, dot_nodename(r->inputs[i]));
        if (is_in_root(r->inputs[i]->path)) {
          fprintf(f, "[label=\"%s\", color=\"red\"];\n",
                  r->inputs[i]->path + lenroot+1);
        } else {
          fprintf(f, "[label=\"%s\", color=\"green\"];\n",
                  r->inputs[i]->path);
        }
        r->inputs[i]->is_printed = true;
      }
    }
  }
}

void fprint_dot(FILE *f, struct all_targets *all) {
  fprintf(f, "digraph G {\n  rankdir=LR;\n");

  for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
    t->is_printed = false;
  }
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }

  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_dot_nodes(f, r);
  }

  fprintf(f, "\n\n");

  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_dot_rule(f, r);
  }
  fprintf(f, "}\n");
}

static void fprint_makefile_rule(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  const int lenroot = strlen(root);

  /* We sort the inputs and outputs before creating the makefile, so
     as to generate output that is the same on every computer, and on
     every invocation.  This is helpful because often one wants to put
     the generated file into the git repository for the benefit of
     users who have not installed fac. */
  struct target **inps = malloc(r->num_inputs*sizeof(struct target *));
  for (int i=0; i<r->num_inputs; i++) inps[i] = r->inputs[i];
  sort_target_array(inps, r->num_inputs);

  struct target **outs = malloc(r->num_outputs*sizeof(struct target *));
  for (int i=0; i<r->num_outputs; i++) outs[i] = r->outputs[i];
  sort_target_array(outs, r->num_outputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (inps[i]->rule) fprint_makefile_rule(f, inps[i]->rule);
  }
  for (int i=0; i<r->num_outputs; i++) {
    fprint_makefile_escape(f, outs[i]->path + lenroot+1);
    fprintf(f, " ");
  }
  fprintf(f, ":");
  for (int i=0; i<r->num_inputs; i++) {
    if (is_in_root(inps[i]->path)) {
      fprintf(f, " ");
      fprint_makefile_escape(f, inps[i]->path + lenroot+1);
    }
  }
  if (is_in_root(r->working_directory)) {
    fprintf(f, "\n\tcd ");
    fprint_makefile_escape(f, r->working_directory + lenroot+1);
    fprintf(f, " && %s\n\n", r->command);
  } else {
    fprintf(f, "\n\t%s\n\n", r->command);
  }
  free(inps);
  free(outs);
}

static void fprint_makeclean_rule(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  const int lenroot = strlen(root);

  /* We sort the outputs before creating the rule, so as to generate
     output that is the same on every computer, and on every
     invocation.  This is helpful because often one wants to put the
     generated file into the git repository for the benefit of users
     who have not installed fac. */
  struct target **inps = malloc(r->num_inputs*sizeof(struct target *));
  for (int i=0; i<r->num_inputs; i++) inps[i] = r->inputs[i];
  sort_target_array(inps, r->num_inputs);

  struct target **outs = malloc(r->num_outputs*sizeof(struct target *));
  for (int i=0; i<r->num_outputs; i++) outs[i] = r->outputs[i];
  sort_target_array(outs, r->num_outputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (inps[i]->rule) fprint_makeclean_rule(f, inps[i]->rule);
  }

  for (int i=0; i<r->num_outputs; i++) {
    fprintf(f, " ");
    fprint_makefile_escape(f, outs[i]->path + lenroot+1);
  }
  free(inps);
  free(outs);
}

void fprint_makefile(FILE *f, struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }

  const int lenroot = strlen(root);
  fprintf(f, "all:");
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    /* First, let's identify whether this rule produces intermediate
       output, in which case it won't need to be included in the "all"
       target. */
    bool is_intermediate = false;
    for (int i=0;i<r->num_outputs;i++) {
      if (r->outputs[i]->num_children) is_intermediate = true;
    }
    if (r->num_outputs && !is_intermediate) {
      /* Only print one output per rule that we are building to save
         space.  Also, if there are no outputs yet (because they are
         not specified in the .fac file, and we have not yet built)
         that is a user error. */
      fprintf(f, " %s", r->outputs[0]->path + lenroot+1);
    } else if (r->status == marked) {
      /* A target was intermediate, but it was explicitly requested,
         so we should list it as part of "all" */
      fprintf(f, " %s", r->outputs[0]->path + lenroot+1);
    }
  }
  fprintf(f, "\n\n");

  /* Now we create the "clean" rule */
  fprintf(f, "clean:\n\trm -f");
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_makeclean_rule(f, r);
  }
  fprintf(f, "\n\n");

  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_makefile_rule(f, r);
  }
}

static void fprint_script_rule(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  const int lenroot = strlen(root);

  /* We sort the inputs and outputs before creating the script, so
     as to generate output that is the same on every computer, and on
     every invocation.  This is helpful because often one wants to put
     the generated file into the git repository for the benefit of
     users who have not installed fac. */
  struct target **inps = malloc(r->num_inputs*sizeof(struct target *));
  for (int i=0; i<r->num_inputs; i++) inps[i] = r->inputs[i];
  sort_target_array(inps, r->num_inputs);

  for (int i=0; i<r->num_inputs; i++) {
    if (inps[i]->rule) fprint_script_rule(f, inps[i]->rule);
  }
  free(inps);
  if (is_in_root(r->working_directory)) {
    fprintf(f, "(cd ");
    fprint_makefile_escape(f, r->working_directory + lenroot+1);
    fprintf(f, " && %s)\n\n", r->command);
  } else {
    fprintf(f, "(%s)\n\n", r->command);
  }
}

void fprint_script(FILE *f, struct all_targets *all) {
  fprintf(f, "#!/bin/sh\n\nset -ev\n\n");
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_script_rule(f, r);
  }
}

static void fprint_tupfile_rule(FILE *f, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  const int lenroot = strlen(root);
  for (int i=0; i<r->num_inputs; i++) {
    if (r->inputs[i]->rule) fprint_tupfile_rule(f, r->inputs[i]->rule);
  }
  fprintf(f, ": ");
  for (int i=0; i<r->num_inputs; i++) {
    if (is_in_root(r->inputs[i]->path)) {
      fprint_makefile_escape(f, r->inputs[i]->path + lenroot+1);
      fprintf(f, " ");
    }
  }
  if (is_in_root(r->working_directory)) {
    fprintf(f, "|> cd ");
    fprint_makefile_escape(f, r->working_directory + lenroot+1);
    fprintf(f, " && %s |>", r->command);
  } else {
    fprintf(f, "|> %s |>", r->command);
  }
  for (int i=0; i<r->num_outputs; i++) {
    fprintf(f, " ");
    fprint_makefile_escape(f, r->outputs[i]->path + lenroot+1);
  }
  fprintf(f, "\n\n");
}

void fprint_tupfile(FILE *f, struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    fprint_tupfile_rule(f, r);
  }
}

static void cp_rule(const char *dir, struct rule *r) {
  if (r->is_printed) return;
  r->is_printed = true;
  const int lenroot = strlen(root);
  for (int i=0; i<r->num_inputs; i++) {
    if (r->inputs[i]->rule) {
      cp_rule(dir, r->inputs[i]->rule);
    } else if (is_in_root(r->inputs[i]->path)) {
      if (r->inputs[i]->is_file) {
        // this is a source file, so we should copy it!
        // printf("cp %s %s/\n", r->inputs[i]->path + lenroot + 1, dir);
        cp_to_dir(r->inputs[i]->path + lenroot + 1, dir);
      }
    }
  }
}

void cp_inputs(const char *dir, struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    r->is_printed = false;
  }
  for (struct rule *r = all->lists[marked]; r; r = r->status_next) {
    cp_rule(dir, r);
  }
}

