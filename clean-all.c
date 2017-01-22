#include "fac.h"
#include "errors.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void clean_all(struct all_targets *all) {
  for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
    t->status = unknown;
    if (is_facfile(t->path)) {
      char *donef = done_name(t->path);
      verbose_printf("rm %s\n", pretty_path(donef));
      unlink(donef);
      free(donef);
    }
  }
  int max_depth = 0;
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    for (int i=0;i<r->num_outputs;i++) {
      if (r->outputs[i]->status == unknown) {
        if (path_depth(r->outputs[i]->path) > max_depth) {
          max_depth = path_depth(r->outputs[i]->path);
        }
        verbose_printf("rm %s\n", pretty_path(r->outputs[i]->path));
        verbose_printf("    %s\n", r->command);
        unlink(r->outputs[i]->path);
      }
    }
  }
  // The following bit is a hokey and inefficient bit of code to
  // ensure that we will rmdir subdirectories prior to their
  // superdirectories.  I don't bother checking if anything is a
  // directory or not, and I recompute depths many times.
  while (max_depth >= 0) {
    for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
      for (int i=0;i<r->num_outputs;i++) {
        if (r->outputs[i]->status == unknown) {
          if (path_depth(r->outputs[i]->path) == max_depth) {
            rmdir(r->outputs[i]->path);
          }
        }
      }
    }
    max_depth--;
  }
}
