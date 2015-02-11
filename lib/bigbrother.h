#include <sys/types.h>

#include "arrayset.h"

/* bigbrother_process executes the requested program and tracks files
   (and directories?) modifed, read and deleted.  It is a blocking
   function, but is intended to be reentrant, so you can run several
   processes simultaneously in different threads.  */
int bigbrother_process(const char *workingdir,
                       pid_t *child_ptr,
                       int stdouterrfd,
                       char **args,
                       arrayset *read_from_directories,
                       arrayset *read_from_files,
                       arrayset *written_to_files,
                       arrayset *deleted_files);
