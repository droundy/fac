#include <sys/types.h>

#include "listset.h"
#include "arrayset.h"

/* bigbrother_process executes the requested program and tracks files
   (and directories?) modifed, read and deleted.  It is a blocking
   function, but is intended to be reentrant, so you can run several
   processes simultaneously in different threads.  */
int bigbrother_process(const char *workingdir,
                       char **args,
                       listset **readdir,
                       listset **read,
                       listset **written,
                       listset **deleted);

int bigbrother_process_arrayset(const char *workingdir,
                                char **args,
                                pid_t *store_child_pid_here,
                                arrayset *readdir,
                                arrayset *read,
                                arrayset *written,
                                arrayset *deleted);
