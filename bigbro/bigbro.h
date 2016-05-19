#include <sys/types.h>

/* bigbrother_process executes the requested program and tracks files
   (and directories?) modified and read.  It is a blocking function,
   but is intended to be reentrant, so you can run several processes
   simultaneously in different threads.  */

int bigbro(const char *workingdir, pid_t *child_ptr, int stdouterrfd,
           char **args, char ***read_from_directories,
           char ***read_from_files, char ***written_to_files);
