#ifndef NEW_BUILD_H
#define NEW_BUILD_H

#include "fac.h"

void mark_facfiles(struct all_targets *all);
void mark_all(struct all_targets *all);
void mark_rule(struct all_targets *all, struct rule *r);

void build_marked(struct all_targets *all);
void check_for_impossibilities(struct all_targets *all);

void build_continual();

void summarize_build_results(struct all_targets *all);

#endif
