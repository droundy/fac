#define _XOPEN_SOURCE 700
#define __BSD_VISIBLE 1
/* the following enables scandir on darwin */
#define _DARWIN_C_SOURCE

#include "errors.h"
#include "fac.h"
#include "build.h"
#include "environ.h"

#include "bigbro.h"
#include "lib/listset.h"

#ifdef _WIN32

#include <direct.h> // for _chdir
#define chdir _chdir
#include <windows.h> // for Sleep
#define sleep Sleep

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>

#ifdef __linux__
#include <sys/inotify.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#endif

#include <semaphore.h>
#include <pthread.h>
#include <math.h>

static listset *facfiles_used = 0;

static  void dump_to_stdout(int fd);

static inline double double_time() {
#ifdef _WIN32
  return GetTickCount()*1e-3;
#else
  struct timeval now;
  gettimeofday(&now, 0);
  return now.tv_sec + now.tv_usec*1e-6;
#endif
}

// The following is used for tracking paths to watch.
struct a_path {
  struct hash_entry e;
  const char path[];
};

// This code determines if a file might be in the git repository.  We
// whitelist the hooks directory, since it is reasonable (or
// semireasonable) for rules to create files in there.
bool is_git_path(const char *path) {
  static char *gitpath = 0, *githookspath = 0;
  static int gitlen = 0, githookslen = 0;
  if (!gitpath) {
    gitpath = absolute_path(root, ".git/");
    gitlen = strlen(gitpath);
  }
  if (!githookspath) {
    githookspath = absolute_path(root, ".git/hooks/");
    githookslen = strlen(githookspath);
  }
  int len = strlen(path);
  if (len > gitlen && memcmp(path, gitpath, gitlen) == 0) {
    if (len > githookslen && memcmp(path, githookspath, githookslen) == 0) {
      return false;
    }
    return true;
  }
  return false;
}

bool is_interesting_path(struct rule *r, const char *path) {
  const int len = strlen(path);
  for (int i=0;i<r->num_cache_prefixes;i++) {
    bool matches = true;
    for (int j=0;r->cache_prefixes[i][j];j++) {
      if (path[j] != r->cache_prefixes[i][j]) {
        matches = false;
        break;
      }
    }
    if (matches) return false;
  }
  for (int i=0;i<r->num_cache_suffixes;i++) {
    bool matches = true;
    for (int j=0;r->cache_suffixes_reversed[i][j];j++) {
      if (path[len-j-1] != r->cache_suffixes_reversed[i][j]) {
        matches = false;
        break;
      }
    }
    if (matches) return false;
  }
  return true;
}

static void check_cleanliness(struct all_targets *all, struct rule *r);

void mark_rule(struct all_targets *all, struct rule *r) {
  if (r->status != unknown) {
    return;
  }
  set_status(all, r, marked);
}

void mark_facfiles(struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    if (r->status == unknown || r->status == unready) {
      for (int i=0;i<r->num_outputs;i++) {
        if (is_facfile(r->outputs[i]->path)) {
          set_status(all, r, unknown);
          mark_rule(all, r);
        }
      }
    }
  }
}
void mark_all(struct all_targets *all) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    if (r->status == unknown && r->is_default) mark_rule(all, r);
  }
}

static void find_latency(struct rule *r) {
  if (r->latency_handled) return;
  r->latency_handled = true;
  double maxchild = 0;
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      find_latency(r->outputs[i]->children[j]);
      if (r->outputs[i]->children[j]->latency_estimate > maxchild)
        maxchild = r->outputs[i]->children[j]->latency_estimate;
    }
  }
  r->latency_estimate = r->build_time + maxchild;
}
void rule_is_ready(struct all_targets *all, struct rule *r) {
  if (false) {
    set_status(all, r, dirty);
  } else {
    /* the following keeps the read_list sorted by latency */

    /* Note that this costs O(N) in the number of ready jobs, and thus
       could be very expensive.  So we probably should throttle this
       feature based on the number of ready jobs. */

    find_latency(r);

    /* remove from its former list */
    if (r->status_prev) {
      (*r->status_prev) = r->status_next;
      all->num_with_status[r->status]--;
    }
    if (r->status_next) {
      r->status_next->status_prev = r->status_prev;
    }

    r->status = dirty;
    all->num_with_status[dirty]++;
    all->estimated_times[dirty] += r->build_time;

    struct rule **list = &all->lists[dirty];
    while (*list && (*list)->latency_estimate > r->latency_estimate) {
      list = &(*list)->status_next;
    }
    r->status_next = *list;
    r->status_prev = list;
    if (r->status_next) r->status_next->status_prev = &r->status_next;
    *list = r;
  }

}
void built_rule(struct all_targets *all, struct rule *r) {
  set_status(all, r, built);
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      if (r->outputs[i]->children[j]->status == unready)
        check_cleanliness(all, r->outputs[i]->children[j]);
    }
  }
}
void rule_failed(struct all_targets *all, struct rule *r) {
  if (r->status == failed) return; /* just in case! */
  r->num_inputs = r->num_explicit_inputs;
  /* We do not want to throw away old non-explicit outputs, since
     these outputs may still exist in the repository. */

  set_status(all, r, failed);
  for (int i=0;i<r->num_outputs;i++) {
    for (int j=0;j<r->outputs[i]->num_children;j++) {
      rule_failed(all, r->outputs[i]->children[j]);
    }
  }
}

static void find_target_sha1(struct target *t, const char *why) {
  if (t->is_file) {
#ifdef _WIN32
    printf("FIXME: I could use stdio here rather than file descriptors.\n");
#else
    int fd = open(t->path, O_RDONLY);
    if (fd > 0) {
      if (false) verbose_printf(" *** sha1sum %s (%s)\n", pretty_path(t->path), why);
      const int bufferlen = 4096*1024;
      char *buffer = malloc(bufferlen);
      int readlen, total_size = 0;
      sha1nfo sh;
      sha1_init(&sh);
      while ((readlen = read(fd, buffer, bufferlen)) > 0) {
        sha1_write(&sh, buffer, readlen);
        total_size += readlen;
      }
      if (readlen == 0) {
        t->stat.hash = sha1_out(&sh);
      }
      close(fd);
      free(buffer);
    }
#endif
  } else if (t->is_symlink) {
#ifdef _WIN32
#else
    int bufferlen = 0;
    char *buffer = 0;
    int readlen;
    do {
      bufferlen += 4096;
      buffer = realloc(buffer, bufferlen);
      readlen = readlink(t->path, buffer, bufferlen);
      if (readlen < 0) {
        free(buffer);
        return;
      }
    } while (readlen > bufferlen);
    sha1nfo sh;
    sha1_init(&sh);
    sha1_write(&sh, buffer, readlen);
    t->stat.hash = sha1_out(&sh);
    free(buffer);
#endif
  } else if (t->is_dir) {
#ifdef _WIN32
#else
    struct dirent **namelist;
    int n;
    n = scandir(t->path, &namelist, NULL, alphasort);
    if (n >= 0) {
      sha1nfo sh;
      sha1_init(&sh);

      for (int i=0; i<n; i++) {
        sha1_write(&sh, namelist[i]->d_name, strlen(namelist[i]->d_name)+1); // write the null byte!
        free(namelist[i]);
      }
      t->stat.hash = sha1_out(&sh);
      free(namelist);
    }
#endif
  }
}

static struct target *create_target_with_stat(struct all_targets *all,
                                              const char *path) {
  struct target *t = create_target(all, path);
  if (!t->stat.time) {
#ifndef _WIN32
    struct stat st;
    if (lstat(t->path, &st)) return 0;
    t->is_file = S_ISREG(st.st_mode);
    t->is_dir = S_ISDIR(st.st_mode);
    t->is_symlink = S_ISLNK(st.st_mode);
    t->stat.size = st.st_size;
    t->stat.time = st.st_mtime;
#endif
  }
  return t;
}

static inline void erase_and_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (isatty(fileno(stdout))) {
    char *total_format = malloc(strlen(format) + 4096);
    sprintf(total_format, "                                                   \r%s", format);
    vfprintf(stdout, total_format, args);
    free(total_format);
  } else {
    vfprintf(stdout, format, args);
  }
  va_end(args);
}

static bool have_announced_rebuild_excuse = false;
static inline void rebuild_excuse(struct rule *r, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (verbose && !have_announced_rebuild_excuse) {
    have_announced_rebuild_excuse = true;
    char *total_format = malloc(strlen(format) + 4096);
    sprintf(total_format, "                                                   \r *** Building %s\n     because %s.\n",
            pretty_rule(r), format);
    vfprintf(stdout, total_format, args);
    free(total_format);
  }
  va_end(args);
}


void check_cleanliness(struct all_targets *all, struct rule *r) {
  if (r->status != unknown && r->status != unready && r->status != marked) {
    /* We have already determined the cleanliness of this rule. */
    return;
  }
  if (r->num_inputs == 0 && r->num_outputs == 0) {
    /* Presumably this means we have never built this rule, and its
       inputs are in git. */
    rule_is_ready(all, r);
    return;
  }
  have_announced_rebuild_excuse = false;
  int old_status = r->status;
  r->status = being_determined;
  bool am_now_unready = false;
  for (int i=0;i<r->num_inputs;i++) {
    if (r->inputs[i]->rule) {
      if (r->inputs[i]->rule->status == unknown ||
          r->inputs[i]->rule->status == marked) {
        check_cleanliness(all, r->inputs[i]->rule);
      }
      if (r->inputs[i]->rule->status == being_determined)
        error_at_line(1, 0, pretty_path(r->facfile_path), r->facfile_linenum,
                      "error: cycle involving %s, %s and %s\n",
                      pretty_rule(r),
                      pretty_path(r->inputs[i]->path),
                      pretty_rule(r->inputs[i]->rule));
      if (r->inputs[i]->rule->status == dirty ||
          r->inputs[i]->rule->status == unready ||
          r->inputs[i]->rule->status == building) {
        am_now_unready = true;
      }
    }
  }
  r->status = old_status;
  if (am_now_unready) {
    set_status(all, r, unready);
    return;
  }
  bool is_dirty = false;
  if (env.abc.a != r->env.abc.a || env.abc.b != r->env.abc.b || env.abc.c != r->env.abc.c) {
    if (r->env.abc.a || r->env.abc.b || r->env.abc.c) {
      rebuild_excuse(r, "the environment has changed");
    } else {
      rebuild_excuse(r, "we have never built it");
    }
    is_dirty = true;
  }
  for (int i=0;i<r->num_inputs;i++) {
    if (!r->inputs[i]->is_in_git && !is_in_gitdir(r->inputs[i]->path) &&
        !r->inputs[i]->rule && is_in_root(r->inputs[i]->path)) {
      if (i < r->num_explicit_inputs) {
        set_status(all, r, unready);
        return;
      } else if (r->inputs[i]->is_file) {
        rebuild_excuse(r, "input %s does not exist", pretty_path(r->inputs[i]->path));
        rule_is_ready(all, r);
        return;
      }
    }
    if (is_dirty) continue; // no need to do the rest now
    if (r->inputs[i]->rule && r->inputs[i]->rule->status == built) {
      if (sha1_is_zero(r->inputs[i]->stat.hash)) find_target_sha1(r->inputs[i], "just built");
      if (sha1_same(r->input_stats[i].hash, r->inputs[i]->stat.hash)) {
        if (false) verbose_printf(" *** hashing saved us work on %s due to rebuild of %s\n",
                                  pretty_rule(r), pretty_path(r->inputs[i]->path));
        r->input_stats[i].time = r->inputs[i]->stat.time;
        r->input_stats[i].size = r->inputs[i]->stat.size;
        insert_to_listset(&facfiles_used, r->facfile_path);
      } else {
        rebuild_excuse(r, "%s has been rebuilt", pretty_path(r->inputs[i]->path));
        is_dirty = true;
      }
    }
    if (r->input_stats[i].time) {
      if (!create_target_with_stat(all, r->inputs[i]->path) ||
          r->input_stats[i].time != r->inputs[i]->stat.time ||
          r->input_stats[i].size != r->inputs[i]->stat.size) {
        if (!sha1_is_zero(r->input_stats[i].hash) && r->input_stats[i].size == r->inputs[i]->stat.size) {
          if (sha1_is_zero(r->inputs[i]->stat.hash)) find_target_sha1(r->inputs[i],
                                                                      "same size input, but zero input_stats");
          if (sha1_same(r->input_stats[i].hash, r->inputs[i]->stat.hash)) {
            if (false) verbose_printf(" *** hashing saved us work on %s due to %s\n",
                                      pretty_rule(r), pretty_path(r->inputs[i]->path));
            r->input_stats[i].time = r->inputs[i]->stat.time;
            r->input_stats[i].size = r->inputs[i]->stat.size;
            insert_to_listset(&facfiles_used, r->facfile_path);
          } else {
            rebuild_excuse(r, "%s is definitely modified", pretty_path(r->inputs[i]->path));
            is_dirty = true;
          }
        } else {
          rebuild_excuse(r, "%s is modified", pretty_path(r->inputs[i]->path));
          is_dirty = true;
        }
      }
    } else {
      rebuild_excuse(r, "#%d %s has no input time",
                     i, pretty_path(r->inputs[i]->path));
      is_dirty = true;
    }
  }
  if (is_dirty) rule_is_ready(all, r);
  for (int i=0;i<r->num_outputs;i++) {
    if (r->output_stats[i].time) {
      if (!create_target_with_stat(all, r->outputs[i]->path) ||
          r->output_stats[i].time != r->outputs[i]->stat.time ||
          r->output_stats[i].size != r->outputs[i]->stat.size) {
        rebuild_excuse(r, "%s has wrong output time",
                       pretty_path(r->outputs[i]->path));
        is_dirty = true;
      }
    } else {
      rebuild_excuse(r, "%s has no output time", pretty_path(r->outputs[i]->path));
      is_dirty = true;
    }
  }
  if (is_dirty) {
    rule_is_ready(all, r);
  } else {
    set_status(all, r, clean);
    if (old_status == unready) {
      for (int i=0;i<r->num_outputs;i++) {
        for (int j=0;j<r->outputs[i]->num_children;j++) {
          if (r->outputs[i]->children[j]->status == unready)
            check_cleanliness(all, r->outputs[i]->children[j]);
        }
      }
    }
  }
}

struct building {
  int all_done;
  pid_t child_pid;
  sem_t *slots_available;
  int stdouterrfd;
  double build_time;
  struct rule *rule;
  char **readdir, **mkdir, **read, **written;
};

static void *run_bigbrother(void *ptr) {
  struct building *b = ptr;

  b->readdir = 0;
  b->mkdir = 0;
  b->read = 0;
  b->written = 0;

  double started = double_time();
  int ret = bigbro_with_mkdir(b->rule->working_directory,
                              &b->child_pid,
                              b->stdouterrfd, b->stdouterrfd, 0,
                              b->rule->command,
                              &b->readdir, &b->mkdir, &b->read, &b->written);

  b->build_time = double_time() - started;
  // memory barrier to ensure b->all_done is not modified before we
  // finish filling everything in:
  __sync_synchronize();
  if (ret != 0) {
    b->all_done = failed;
  } else {
    b->all_done = built;
  }
  sem_post(b->slots_available);
  pthread_exit(0);
  return 0;
}

static struct building *build_rule(struct all_targets *all,
                                   struct rule *r,
                                   sem_t *slots_available,
                                   const char *log_directory) {
  struct building *b = malloc(sizeof(struct building));
  b->rule = r;
  b->slots_available = slots_available;
  b->stdouterrfd = -1; /* start with an invalid value */
  if (log_directory) {
    char *path = absolute_path(root, log_directory);
    add_cache_prefix(r, path);
    free(path);

    const char *rname = pretty_rule(r);
    char *fname = malloc(strlen(log_directory) + strlen(rname)+2);
#ifdef _WIN32
    _mkdir(log_directory);
#else
    mkdir(log_directory, 0777); // ignore failure
#endif
    strcpy(fname, log_directory);
    int start = strlen(log_directory);
    fname[start++] = '/';
    const int rulelen = strlen(rname);
    for (int i=0;i<rulelen;i++) {
      switch (rname[i]) {
      case '/':
      case ' ':
      case '"':
      case '\'':
        fname[start+i] = '_';
        break;
      default:
        fname[start+i] = rname[i];
      }
    }
    fname[start+rulelen] = 0;
#ifdef _WIN32
    printf("Do something here!\n");
#else
    b->stdouterrfd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0666);
#endif
    free(fname);
  }
  if (b->stdouterrfd == -1) {
#ifdef _WIN32
    printf("Figure out how to create a temp file! Or just plan on reading from pipe?\n");
#else
    const char *templ = "/tmp/fac-XXXXXX";
    char *namebuf = malloc(strlen(templ)+1);
    strcpy(namebuf, templ);
    b->stdouterrfd = mkstemp(namebuf);
    unlink(namebuf);
    free(namebuf);
#endif
  }
  b->all_done = building;
  set_status(all, r, building);

  /* Now we go ahead and actually start running the build. */
  pthread_t child = 0;
  pthread_create(&child, 0, run_bigbrother, b);
  pthread_detach(child);

  return b;
}

static double starting_time;
static double elapsed_seconds, elapsed_minutes;
static bool am_interrupted = false;

void initialize_starting_time() {
  starting_time = double_time();
}

#ifndef _WIN32
static void handle_interrupt(int sig) {
  am_interrupted = true;
}
#endif

static void find_elapsed_time() {
  double et = double_time() - starting_time;
  elapsed_minutes = floor(et/60);
  elapsed_seconds = et - elapsed_minutes*60;
}

static void delete_from_array(char **array, const char *str) {
  for (int i=0; array[i]; i++) {
    if (!strcmp(array[i], str)) {
      array[i] = "/dev/null"; // hokey hokey way to delete an entry
    }
  }
}

/**
 * Extract parent path from given filename.
 */
static char* extract_parent_dirname(const char *fname) {
  char *dirname = malloc(strlen(fname)+1);
  strcpy(dirname, fname);
  if (strrchr(dirname, '/')) {
    *strrchr(dirname, '/') = 0; // remove last segment of dirname
    return dirname;
  }
  free(dirname);
  return NULL;
}

/**
 * Find all input/output targets for all rules and update their timestamps
 * to given target if they match.
 */
static void update_stats_for_target(struct all_targets *all, struct target *t) {
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    for (int i=0;i<r->num_outputs;i++) {
      if (r->outputs[i] == t) {
        if (r->output_stats[i].time != t->stat.time) {
          r->output_stats[i].time = t->stat.time;
        }
        break;
      }
    }
    for (int i=0;i<r->num_inputs;i++) {
      if (r->inputs[i] == t) {
        if (r->input_stats[i].time != t->stat.time) {
          r->input_stats[i].time = t->stat.time;
        }
        break;
      }
    }
  }
}

/**
 * Adding/removing children of a directory causes the parent directory timestamp to 
 * be updated, causing inconsistencies later when reading cached stats from the .tum
 * files since they'll be older than the on-disk timestamp. This can cause unnecessary
 * rebuilds even when nothing really has changed. The following update_parent_dir()
 * function should be called by build_marked() whenever a target is modified so that we
 * can update the timestamps of its parent directory (if any).
 *
 * Update parent directory target (if any) timestamp to be the same as the given
 * target, and then update the timestamps in associated stats as well.
 */
static void update_parent_dir(struct all_targets *all, const char *path, struct target *t) {
  char* parent_path = extract_parent_dirname(path);
  if (parent_path) {
    struct target* parent_target = create_target_with_stat(all, parent_path);
    /* Only proceed if parent is really a directory target. */
    if (parent_target && parent_target->is_dir) {
      if (parent_target->stat.time != t->stat.time) {
        parent_target->stat.time = t->stat.time;
      }
      /* Update associated stats for this parent target. */
      update_stats_for_target(all, parent_target);
    }
    free(parent_path);
  }
}

static void build_marked(struct all_targets *all, const char *log_directory,
                         bool git_add_files, bool happy_building_at_least_one,
                         bool ignore_missing_files, struct hash_table *files_to_watch) {
  int num_built_when_we_started = all->num_with_status[built];
  if (!all->lists[marked] && !all->lists[dirty]) {
    if (all->num_with_status[failed]) {
      printf("Failed to build %d files.\n", all->num_with_status[failed]);
      exit(1);
    }
    return; /* nothing to build */
  }

  if (!num_jobs) {
#ifdef _WIN32
    num_jobs = 1;
#else
    num_jobs = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    verbose_printf("Using %d jobs\n", num_jobs);
  }

  struct building **bs = malloc(num_jobs*sizeof(struct building *));
  sem_t *slots_available = malloc(sizeof(sem_t));
  if (sem_init(slots_available, 0, 0) == -1) {
    error(1, errno, "unable to create semaphore");
  }
  for (int i=0;i<num_jobs;i++) bs[i] = 0;
#ifndef _WIN32
  struct sigaction act, oldact;
  act.sa_handler = handle_interrupt;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, &oldact);
#endif

  for (struct rule *r = all->lists[marked]; r; r = all->lists[marked]) {
    check_cleanliness(all, r);
  }

  do {
    int threads_in_use = 0;
    for (int i=0;i<num_jobs;i++) {
      if (bs[i]) threads_in_use++;
    }

    if (threads_in_use) {
      sem_wait(slots_available); // wait for *someone* to finish
      sem_post(slots_available); // to get the counting right.
      for (int i=0;i<num_jobs;i++) {
        if (bs[i] && bs[i]->all_done != building) {
          sem_wait(slots_available);
          threads_in_use--;

          all->estimated_times[bs[i]->rule->status] -= bs[i]->rule->build_time;
          bs[i]->rule->old_build_time = bs[i]->rule->build_time;
          bs[i]->rule->build_time = bs[i]->build_time;
          all->estimated_times[bs[i]->rule->status] += bs[i]->rule->build_time;
          /* the blank spaces below clear out the progress message */
          const int num_built = 1 + all->num_with_status[failed] + all->num_with_status[built];
          const int num_total = all->num_with_status[failed] + all->num_with_status[built] +
            all->num_with_status[building] + all->num_with_status[dirty] +
            all->num_with_status[unready];
          if (bs[i]->build_time < 10) {
            erase_and_printf("%d/%d [%.2fs]: %s\n",
                             num_built, num_total, bs[i]->build_time, bs[i]->rule->command);
          } else if (bs[i]->build_time < 100) {
            erase_and_printf("%d/%d [%.1fs]: %s\n",
                             num_built, num_total, bs[i]->build_time, bs[i]->rule->command);
          } else {
            erase_and_printf("%d/%d [%.0fs]: %s\n",
                             num_built, num_total, bs[i]->build_time, bs[i]->rule->command);
          }
          double estimated_time = (all->estimated_times[building] +
                                   all->estimated_times[dirty] +
                                   all->estimated_times[unready])/num_jobs;
          if (estimated_time > 1.0) {
            int build_minutes = (int)(estimated_time/60);
            double build_seconds = estimated_time - 60*build_minutes;
            find_elapsed_time();
            double total_seconds = elapsed_seconds + build_seconds;
            double total_minutes = elapsed_minutes + build_minutes;
            if (total_seconds > 60) {
              total_minutes += 1;
              total_seconds -= 60;
            }
            if (isatty(fileno(stdout))) {
              erase_and_printf("Build time remaining: %d:%02.0f / %.0f:%02.0f\r",
                               build_minutes, build_seconds, total_minutes, total_seconds);
            } else {
              erase_and_printf("Build time remaining: %d:%02.0f / %.0f:%02.0f\n",
                               build_minutes, build_seconds, total_minutes, total_seconds);
            }
            fflush(stdout);
          }
          if (bs[i]->all_done != built || show_output) {
            dump_to_stdout(bs[i]->stdouterrfd);
            close(bs[i]->stdouterrfd);
          }
          if (bs[i]->all_done != built && bs[i]->all_done != failed) {
            erase_and_printf("INTERRUPTED!  bs[i]->all_done == %s\n",
                             pretty_status(bs[i]->all_done));
            am_interrupted = true;
            break;
          }
          if (files_to_watch) {
            for (int nn=0; bs[i]->read[nn]; nn++) {
              if (!lookup_in_hash(files_to_watch, bs[i]->read[nn])) {
                struct a_path *foo = malloc(sizeof(struct a_path) + strlen(bs[i]->read[nn])+1);
                foo->e.key = foo->path;
                strcpy((char *)foo->path, bs[i]->read[nn]);
                foo->e.next = 0;
                add_to_hash(files_to_watch, &foo->e);
              }
            }
            for (int nn=0; bs[i]->readdir[nn]; nn++) {
              if (!lookup_in_hash(files_to_watch, bs[i]->readdir[nn])) {
                struct a_path *foo = malloc(sizeof(struct a_path) + strlen(bs[i]->readdir[nn])+1);
                foo->e.key = foo->path;
                strcpy((char *)foo->path, bs[i]->readdir[nn]);
                foo->e.next = 0;
                add_to_hash(files_to_watch, &foo->e);
              }
            }
          }

          if (bs[i]->all_done == failed) {
            rule_failed(all, bs[i]->rule);
            erase_and_printf("build failed: %s\n", pretty_rule(bs[i]->rule));
            free(bs[i]->read);
            free(bs[i]->readdir);
            for (int nn=0; bs[i]->written[nn]; nn++) {
              // Delete any files that were created, so that they will
              // be properly re-created next time this command is run.
              unlink(bs[i]->written[nn]);
            }
            free(bs[i]->written);
            for (int nn=0; bs[i]->mkdir[nn]; nn++) {
              // Delete any files that were created, so that they will
              // be properly re-created next time this command is run.
              rmdir(bs[i]->mkdir[nn]);
            }
            free(bs[i]->mkdir);
            free(bs[i]);
            bs[i] = 0;
            break;
          }

          struct rule *r = bs[i]->rule;
          insert_to_listset(&facfiles_used, r->facfile_path);

          /* First, we want to save as many of the old inputs as
             possible. */
          for (int ii=0; ii<r->num_inputs; ii++) {
            struct target *t = create_target_with_stat(all, r->inputs[ii]->path);
            if (t
                && sha1_is_zero(t->stat.hash)
                && t->stat.time == r->input_stats[ii].time
                && t->stat.size == r->input_stats[ii].size) {
              /* Assume with same modification time and size that the
                 file contents are not changed. */
              t->stat.hash = r->input_stats[ii].hash;
            }
          }
          /* Forget the non-explicit imputs, as we will re-add those
             inputs that were actually used in the build */
          r->num_inputs = r->num_explicit_inputs;
          for (int ii=0;ii<r->num_inputs;ii++) {
            struct target *t = create_target_with_stat(all, r->inputs[ii]->path);
            if (!t) {
              error(1, 0, "Unable to stat input file %s (this should be impossible)\n",
                    r->inputs[ii]->path);
            } else {
              if (sha1_is_zero(t->stat.hash)) find_target_sha1(t, "we have no hash");
              add_input(r, t);
              delete_from_array(bs[i]->read, r->inputs[ii]->path);
              delete_from_array(bs[i]->written, r->inputs[ii]->path);
            }
          }
          for (int ii=0;ii<r->num_outputs;ii++) {
            struct target *t = lookup_target(all, r->outputs[ii]->path);
            if (t) {
              t->stat.time = 0;
              t->stat.size = 0;
            }
            t = create_target_with_stat(all, r->outputs[ii]->path);
            if (!t && ii < r->num_explicit_outputs) {
              erase_and_printf("build failed to create: %s\n",
                               pretty_path(r->outputs[ii]->path));
              rule_failed(all, r);
              if (!show_output) {
                dump_to_stdout(bs[i]->stdouterrfd);
                close(bs[i]->stdouterrfd);
              }
              free(bs[i]->read);
              free(bs[i]->readdir);
              free(bs[i]->written);
              free(bs[i]->mkdir);
              free(bs[i]);
              bs[i] = 0;
              break;
            } else if (!t) {
              /* This file was previously created, but is no longer
                 there, so we should remove it from the list of
                 outputs. */
              r->outputs[ii]->rule = 0; // dissociate ourselves with this output
              r->outputs[ii]->status = dirty; // mark it as dirty, since we didn't create it

              r->num_outputs -= 1;
              for (int j=ii;j<r->num_outputs;j++) {
                r->outputs[j] = r->outputs[j+1];
              }
              ii -= 1; /* we need to do "ii" one more time, since we
                          shifted everything else back */
            } else {
              find_target_sha1(t, "t");
              t->rule = r;
              add_output(r, t);
              delete_from_array(bs[i]->read, r->outputs[ii]->path);
              delete_from_array(bs[i]->written, r->outputs[ii]->path);
              delete_from_array(bs[i]->mkdir, r->outputs[ii]->path);
            }
          }
          if (!bs[i]) break; // happens if we failed to create an output

          for (int nn=0; bs[i]->read[nn]; nn++) {
            char *path = bs[i]->read[nn];
            if (is_interesting_path(r, path)) {
              struct target *t = create_target_with_stat(all, path);
              if (!t) {
                /* Assume that the file was deleted, and there's no
                   problem. */
                // error(1, errno, "Unable to input stat file %s", path);
              } else {
                if (!t->rule && is_in_root(path) && !t->is_in_git && !is_in_gitdir(path)) {
                  if (git_add_files) {
                    git_add(path);
                    t->is_in_git = true;
                  } else {
                    erase_and_printf("error: %s should be in git for %s\n",
                                     pretty_path(t->path), pretty_rule(r));
                    rule_failed(all, r);
                  }
                }
                if (sha1_is_zero(t->stat.hash)) find_target_sha1(t, "fixed case");
                add_input(r, t);
              }
            }
          }

          for (int nn=0; bs[i]->readdir[nn]; nn++) {
            char *path = bs[i]->readdir[nn];
            if (is_interesting_path(r, path)) {
              struct target *t = create_target_with_stat(all, path);
              if (t && t->is_dir) {
                find_target_sha1(t, "created new");
                add_input(r, t);
              }
            }
          }

          for (int nn=0; bs[i]->written[nn]; nn++) {
            char *path = bs[i]->written[nn];
            // The following ignores writes to files in the .git/
            // directory, in order to avoid situations where running
            // fac -c cleans up such files, causing repository
            // corruption.  It also ignores paths that have been
            // marked as "cache" paths by the user.
            if (!is_git_path(path) && is_interesting_path(r, path)) {
              struct target *t = lookup_target(all, path);
              if (t) {
                t->stat.time = 0;
                t->stat.size = 0;
              }
              t = create_target_with_stat(all, path);
              if (t) {
                update_parent_dir(all, path, t);
                if (t->is_file || t->is_symlink) {
                  if (path == pretty_path(path)) {
                    erase_and_printf("error: created file outside source directory: %s (%s)\n",
                                     path, pretty_rule(r));
                    rule_failed(all, r);
                  }
                  if (t->rule && t->rule != r) {
                    erase_and_printf("error: two rules generate same output %s:\n\t%s\nand\n\t%s\n",
                                     pretty_path(path), r->command, t->rule->command);
                    rule_failed(all, r);
                  }
                  t->rule = r;
                  t->status = unknown; // if it is a facfile, we haven't yet read it
                  find_target_sha1(t, "new output");
                  add_output(r, t);
                }
              }
            }
          }
          for (int nn=0; bs[i]->mkdir[nn]; nn++) {
            char *path = bs[i]->mkdir[nn];
            // The following ignores creation of directories in the
            // .git/ directory, in order to avoid situations where
            // running fac -c cleans up such directories, causing
            // repository corruption.  It also ignores paths that have
            // been marked as "cache" paths by the user.
            if (!is_git_path(path) && is_interesting_path(r, path)) {
              struct target *t = lookup_target(all, path);
              if (t) {
                t->stat.time = 0;
                t->stat.size = 0;
              }
              t = create_target_with_stat(all, path);
              if (t) {
                update_parent_dir(all, path, t);
                if (t->is_dir) {
                  if (path == pretty_path(path)) {
                    erase_and_printf("error: created directory outside source directory: %s (%s)\n",
                                     path, pretty_rule(r));
                    rule_failed(all, r);
                  }
                  if (t->rule && t->rule != r) {
                    erase_and_printf("error: two rules generate same output %s:\n\t%s\nand\n\t%s\n",
                                     pretty_path(path), r->command, t->rule->command);
                    rule_failed(all, r);
                  }
                  t->rule = r;
                  t->status = unknown; // if it is a facfile, we haven't yet read it
                  find_target_sha1(t, "mkdir output");
                  add_output(r, t);
                }
              }
            }
          }
          if (r->status != failed) {
            r->env = env; /* save the environment for this rule */
            built_rule(all, r);
          }
          insert_to_listset(&facfiles_used, r->facfile_path);

          if (!show_output) close(bs[i]->stdouterrfd);
          free(bs[i]->read);
          free(bs[i]->readdir);
          free(bs[i]->written);
          free(bs[i]->mkdir);
          free(bs[i]);
          bs[i] = 0;
        }
      }
    }

    {
      int N = num_jobs - threads_in_use;
      struct rule **toqueue = calloc(N, sizeof(struct rule *));
      int i = 0;
      for (struct rule *r = all->lists[dirty]; r && i < N; r = r->status_next) {
        toqueue[i++] = r;
      }
      for (i=0;i<N;i++) {
        if (toqueue[i]) {
          for (int j=0;j<num_jobs;j++) {
            if (!bs[j]) {
              bs[j] = build_rule(all, toqueue[i],
                                 slots_available, log_directory);
              break;
            }
          }
        }
      }
      free(toqueue);
    }
    if (am_interrupted) {
      for (int i=0;i<num_jobs;i++) {
        if (bs[i]) {
          verbose_printf("killing %d (%s)\n", bs[i]->child_pid,
                         pretty_rule(bs[i]->rule));
#ifdef _WIN32
          erase_and_printf("do not know how to kill %d\n", (int)-bs[i]->child_pid);
#else
          kill(-bs[i]->child_pid, SIGTERM); /* ask child to die */
#endif
        }
      }
      sleep(1); /* give them a second to die politely */
      for (int i=0;i<num_jobs;i++) {
        if (bs[i]) {
#ifdef _WIN32
          erase_and_printf("do not know how to kill %d\n", (int)-bs[i]->child_pid);
#else
          kill(-bs[i]->child_pid, SIGKILL); /* kill with extreme prejudice */
#endif
          free(bs[i]);
          bs[i] = 0;
        }
      }
      erase_and_printf("Interrupted!\n");

      while (facfiles_used) {
        char *donefile = done_name(facfiles_used->path);
        FILE *f = fopen(donefile, "w");
        if (!f) error(1,errno,"oopse");
        fprint_facfile(f, all, facfiles_used->path);
        fclose(f);
        free(donefile);
        listset *to_delete = facfiles_used;
        facfiles_used = facfiles_used->next;
        free(to_delete->path);
        free(to_delete);
      }

      exit(1);
    }
  } while (all->lists[dirty] || all->lists[building]);

  int num_finally_built = all->num_with_status[built];
  for (struct rule *r = all->lists[unready]; r; r = all->lists[unready]) {
    if ((happy_building_at_least_one && num_built_when_we_started != num_finally_built)
        || ignore_missing_files) {
      set_status(all, r, unknown);
    } else {
      for (int i=0;i<r->num_inputs;i++) {
        if (!r->inputs[i]->rule && !r->inputs[i]->is_in_git &&
            !is_in_gitdir(r->inputs[i]->path) && is_in_root(r->inputs[i]->path)) {
          if (!access(r->inputs[i]->path, R_OK)) {
            if (git_add_files) {
              git_add(r->inputs[i]->path);
            } else {
              erase_and_printf("error: add %s to git, which is required for %s\n",
                               pretty_path(r->inputs[i]->path), pretty_reason(r));
            }
          } else {
            erase_and_printf("error: missing file %s, which is required for %s\n",
                             pretty_path(r->inputs[i]->path), pretty_reason(r));
          }
        }
      }
      rule_failed(all, r);
    }
  }

  while (facfiles_used) {
    char *donefile = done_name(facfiles_used->path);
    FILE *f = fopen(donefile, "w");
    if (!f) error(1,errno,"oopsies");
    fprint_facfile(f, all, facfiles_used->path);
    fclose(f);
    free(donefile);
    listset *to_delete = facfiles_used;
    facfiles_used = facfiles_used->next;
    free(to_delete->path);
    free(to_delete);
  }

  free(slots_available);
  free(bs);
#ifndef _WIN32
  sigaction(SIGINT, &oldact, 0);
#endif
}

void summarize_build_results(struct all_targets *all, bool am_continual) {
  if (all->lists[failed] || all->num_with_status[failed]) {
    erase_and_printf("Build failed %d/%d failures\n", all->num_with_status[failed],
                     all->num_with_status[failed] + all->num_with_status[built]);
    fflush(stdout);
    if (am_continual) {
      printf("Waiting for change to try again...\n");
    } else {
      exit(1);
    }
  } else {
    find_elapsed_time();
    erase_and_printf("Build succeeded! %.0f:%05.2f\n",
                     elapsed_minutes, elapsed_seconds);
    fflush(stdout);
  }
}

sha1hash env;

void do_actual_build(struct cmd_args *args) {
  env = hash_environment();
  do {
    struct all_targets all;
    init_all(&all);

    struct hash_table *files_to_watch = 0;
    if (args->continual) {
      files_to_watch = (struct hash_table *)malloc(sizeof(struct hash_table));
      init_hash_table(files_to_watch, 2048);
    }

    bool still_reading;
    do {
      still_reading = false;
      for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
        if (t->status == unknown &&
            (!t->rule ||
             t->rule->status == clean ||
             t->rule->status == built)) {
          t->status = built;
          if (is_facfile(t->path)) {
            still_reading = true;
            read_fac_file(&all, t->path);
          }
        }
      }
      mark_facfiles(&all);
      build_marked(&all, args->log_directory, args->git_add_files, true, still_reading,
                   files_to_watch);
      for (struct target *t = (struct target *)all.t.first; t; t = (struct target *)t->e.next) {
        if (t->status == unknown &&
            (!t->rule ||
             t->rule->status == clean ||
             t->rule->status == built)) {
          if (is_facfile(t->path)) still_reading = true;
        }
      }
    } while (still_reading);
    if (!all.r.first) {
      erase_and_printf("Please add a .fac file containing rules!\n");
      exit(1);
    }
    if (args->clean) {
      clean_all(&all);
      exit(0);
    }
    if (args->targets_requested) {
      for (listset *a = args->targets_requested; a; a = a->next) {
        struct target *t = lookup_target(&all, a->path);
        if (t && t->rule) {
          mark_rule(&all, t->rule);
        } else {
          error(1, 0, "No rule to build %s\n", pretty_path(a->path));
        }
      }
    } else {
      mark_all(&all);
    }

    build_marked(&all, args->log_directory, args->git_add_files, false, false,
                 files_to_watch);
    summarize_build_results(&all, args->continual);

    if (args->create_dotfile || args->create_makefile || args->create_tupfile
        || args->create_script || args->create_tarball) {
      for (struct rule *r = (struct rule *)all.r.first; r; r = (struct rule *)r->e.next) {
        set_status(&all, r, unknown);
      }
      if (args->targets_requested) {
        for (listset *a = args->targets_requested; a; a = a->next) {
          struct target *t = lookup_target(&all, a->path);
          if (t && t->rule) {
            mark_rule(&all, t->rule);
          } else {
            error(1, 0, "No rule to build %s", pretty_path(a->path));
          }
        }
      } else {
        mark_all(&all);
      }

      if (args->create_dotfile) {
        FILE *f = fopen(args->create_dotfile, "w");
        if (!f) error(1,errno, "Unable to create dotfile: %s", args->create_dotfile);
        fprint_dot(f, &all);
        fclose(f);
      }
      if (args->create_tarball) {
        // If we're creating a tarball, we want to generate build
        // files that do not try to rebuild things that we are
        // including in the tarball.  Note that this is a horrible,
        // ugly, hack.
        for (int i=0; args->include_in_tar[i]; i++) {
          struct target *t = lookup_target(&all, absolute_path(root, args->include_in_tar[i]));
          if (t) {
            t->rule = 0;
            t->is_in_git = true;
          }
        }
      }
      if (args->create_makefile) {
        FILE *f = fopen(args->create_makefile, "w");
        if (!f) error(1,errno, "Unable to create makefile: %s", args->create_makefile);
        fprint_makefile(f, &all);
        fclose(f);
      }
      if (args->create_tupfile) {
        FILE *f = fopen(args->create_tupfile, "w");
        if (!f) error(1,errno, "Unable to create tupfile: %s", args->create_tupfile);
        fprint_tupfile(f, &all);
        fclose(f);
      }
      if (args->create_script) {
        FILE *f = fopen(args->create_script, "w");
        if (!f) error(1,errno, "Unable to create script: %s", args->create_script);
        fprint_script(f, &all);
        fclose(f);
      }
      if (args->create_tarball) {
        char *dirname = malloc(strlen(args->create_tarball)+1);
        const char *flags  = "cf";
        int dirlen = strlen(args->create_tarball)-4;
        if (dirlen < 1) {
          fprintf(stderr, "invalid tar filename: %s", args->create_tarball);
          exit(1);
        }
        if (dirlen > 3 && strcmp(args->create_tarball+dirlen-3, ".tar.gz") == 0) {
          flags = "czf";
          dirlen -= 3;
        } else if (dirlen > 0 && strcmp(args->create_tarball+dirlen, ".tgz") == 0) {
          flags = "czf";
        } else if (dirlen > 4 && strcmp(args->create_tarball+dirlen-4, ".tar.bz2") == 0) {
          flags = "cjf";
          dirlen -= 4;
        }
        strncpy(dirname, args->create_tarball, dirlen);
        dirname[dirlen] = 0;
        rm_recursive(dirname); // ignore errors, which are probably does-not-exist
#ifdef _WIN32
        int mkdir_error = _mkdir(dirname);
#else
        int mkdir_error = mkdir(dirname, 0777); // ignore failure
#endif

        if (mkdir_error) {
          error(1,errno, "Unable to create tar directory: %s", dirname);
        }
        for (int i=0; args->include_in_tar[i]; i++) {
          cp_to_dir(args->include_in_tar[i], dirname);
        }
        cp_inputs(dirname, &all);
        if (args->create_script) cp_to_dir(args->create_script, dirname);
        if (args->create_makefile) cp_to_dir(args->create_makefile, dirname);
        if (args->create_tupfile) cp_to_dir(args->create_tupfile, dirname);
        // fixme: avoid using system in favor of fork+exec!
        char *cmdline = malloc(4096);
        snprintf(cmdline, 4096, "tar %s '%s' '%s'", flags, args->create_tarball, dirname);
        if (system(cmdline)) {
          printf("Error running tar!\n");
        }
        rm_recursive(dirname);
        free(dirname);
        free(cmdline);
      }
    }

#ifdef __linux__
    if (args->continual) {
      int ifd = inotify_init1(IN_CLOEXEC);
      for (struct target *t=(struct target *)all.t.first; t; t = (struct target *)t->e.next) {
        t->status = unknown;
        if (t->num_children || is_facfile(t->path)) {
          inotify_add_watch(ifd, t->path, IN_CLOSE_WRITE | IN_DELETE_SELF);
        }
      }
      for (int i=0; i<files_to_watch->size; i++) {
        if (files_to_watch->table[i]) {
          inotify_add_watch(ifd, files_to_watch->table[i]->key,
                            IN_CLOSE_WRITE | IN_DELETE_SELF);
        }
      }
      for (struct rule *r = (struct rule *)all.r.first; r; r = (struct rule *)r->e.next) {
        r->status = unknown;
      }
      /* Wait until we have seen a file change.  If we were careful, we
         could check which file was changed, and trigger a minimal
         rebuild without scanning the entire tree again.  But instead, I
         am just rerunning the entire build process. */
      char *buffer = malloc(sizeof(struct inotify_event) + 8192 + 1);
      if (read(ifd, buffer, sizeof(struct inotify_event) + 8192 + 1) <= 0) {
        error(1, errno, "trouble waiting for a file to change.");
      }
      free(buffer);
      close(ifd);
    }
#endif

    free_all_targets(&all);
    if (files_to_watch) {
      // the following is a hokey and sloppy way to free all the
      // entries, but freeing a linked list is a little tedious.
      for (int i=0; i<files_to_watch->size; i++) {
        if (files_to_watch->table[i]) free(files_to_watch->table[i]);
      }
      free_hash_table(files_to_watch);
      free(files_to_watch);
    }
  } while (!am_interrupted && args->continual);

  free_listset(args->targets_requested); // avoid harmless memory leak
}

static void dump_to_stdout(int fd) {
  lseek(fd, 0, SEEK_SET);
  size_t mysize = 0;
  void *buf = malloc(4096);
  while ((mysize = read(fd, buf, 4096)) > 0) {
    if (write(1, buf, mysize) != mysize) {
      erase_and_printf("\nerror: trouble writing to stdout!\n");
    }
  }
  free(buf);
}
