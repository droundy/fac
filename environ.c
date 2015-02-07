#include "environ.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

extern char **environ;

static int cmpstr(const void *p1, const void *p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static const char *ignore_these[] = {
  "DISPLAY=",
  "GNOME_KEYRING_PID=",
  "GPG_AGENT_INFO=",
  "XDG_SESSION_COOKIE=",
  "XAUTHORITY=",
  "PS1=",
  "PWD=",
  "OLDPWD=",
  "WINDOWID=",
  "WINDOWPATH=",
  "TERM=",
  "SSH_AGENT_PID=",
  "SSH_AUTH_SOCK=",
  "_=",
  0
};

sha1hash hash_environment() {
  bool done_removing = true;
  do {
    const char **to_ignore = &ignore_these[0];
    while (*to_ignore) {
      char **e = environ;
      while (*e) {
        if (!strncmp(*to_ignore, *e, strlen(*to_ignore))) {
          /* delete this environment variable */
          while ((*e = *(e+1))) e++;
          break;
        }
        e++;
      }
      to_ignore++;
    }
  } while (!done_removing);

  sha1nfo sh;
  sha1_init(&sh);
  size_t num_env = 0; /* count environment variables */
  for (num_env = 0;environ[num_env];num_env++);
  /* sort the environment variables, so we will get a reproducible
     hash */
  qsort(environ, num_env, sizeof(char *), cmpstr);
  for (int i=0;i<num_env;i++) {
    sha1_write(&sh, environ[i], strlen(environ[i])+1);
    if (false) printf("env: %s\n", environ[i]); /* uncomment for debugging */
  }
  return sha1_out(&sh);
}
