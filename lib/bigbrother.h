#include <sys/types.h>

#include "arrayset.h"

/* bigbrother_process executes the requested program and tracks files
   (and directories?) modifed, read and deleted.  It is a blocking
   function, but is intended to be reentrant, so you can run several
   processes simultaneously in different threads.  */
int bigbrother_process_arrayset(const char *workingdir,
                                char **args,
                                arrayset *readdir,
                                arrayset *read,
                                arrayset *written,
                                arrayset *deleted);
