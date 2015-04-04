#define _XOPEN_SOURCE 700

#include "fac.h"
#include "errors.h"

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32

#include <windows.h>
#include <direct.h> // for _getcwd
#define getcwd _getcwd
#include <process.h> // for spawnvp

int ReadChildProcess(char **output) {
  HANDLE g_hChildStd_OUT_Rd = NULL;
  HANDLE g_hChildStd_OUT_Wr = NULL;

  SECURITY_ATTRIBUTES sa;
  printf("\n->Start of parent execution.\n");
  // Set the bInheritHandle flag so pipe handles are inherited.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;
  // Create a pipe for the child process's STDOUT.
  if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0) ) {
    exit(1);
  }
  // Ensure the read handle to the pipe for STDOUT is not inherited
  if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ){
    exit(1);
  }

  // Create a child process that uses the previously created pipe
  // for STDOUT.

  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  bool bSuccess = FALSE;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDOUT handle for redirection.
  ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
  siStartInfo.cb = sizeof(STARTUPINFO);
  //siStartInfo.hStdError = g_hChildStd_OUT_Wr;
  siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the child process.
  bSuccess = CreateProcess(NULL,
                           "git ls-files",// command line
                           NULL,          // process security attributes
                           NULL,          // primary thread security attributes
                           TRUE,          // handles are inherited
                           0,             // creation flags
                           NULL,          // use parent's environment
                           NULL,          // use parent's current directory
                           &siStartInfo,  // STARTUPINFO pointer
                           &piProcInfo);  // receives PROCESS_INFORMATION
  CloseHandle(g_hChildStd_OUT_Wr);
  // If an error occurs, exit the application.
  if ( ! bSuccess ) {
    exit(1);
  }

  // Read output from the child process's pipe for STDOUT
  // and write to the parent process's pipe for STDOUT.
  // Stop when there is no more data.
  DWORD dwRead = 0;
  int bufsize = 1024;
  *output = malloc(bufsize);
  int read_offset = 0;
  do {
    bSuccess=ReadFile( g_hChildStd_OUT_Rd, *output + read_offset,
                       bufsize-read_offset-1, &dwRead, NULL);

    read_offset += dwRead; // advance pointer to read location for next read
    (*output)[read_offset] = 0; // null-terminate output
    if (bufsize - read_offset < 6) {
      bufsize *= 2;
      *output = realloc(*output, bufsize);
    }
  } while (bSuccess && dwRead);

  // To avoid resource leaks close handle explicitly.
  CloseHandle(g_hChildStd_OUT_Rd);

  // Wait until child process exits.
  WaitForSingleObject( piProcInfo.hProcess, INFINITE );
  int result = -1;
  if(!GetExitCodeProcess(piProcInfo.hProcess,(LPDWORD)&result)) {
    int myerrno = GetLastError();
    error(1, myerrno, "GetExitCodeProcess() failed");
  }

  return result;
}

#else

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#endif

char *go_to_git_top() {
  while (1) {
    char *dirname = getcwd(0,0);

    if (strcmp(dirname, "/") == 0) {
      fprintf(stderr, "error: could not locate .git!\n  %s", strerror(errno));
      exit(1);
    }
    if (!access(".git", R_OK | X_OK)) {
      return dirname;
    }

    if (chdir("..")) {
      fprintf(stderr, "error: unable to chdir(..) from %s\n  %s", dirname, strerror(errno));
      exit(1);
    }
    free(dirname);
  }
}

void add_git_files(struct all_targets *all) {
#ifdef _WIN32
  char *buf = 0;
  int retval = ReadChildProcess(&buf);
  if (retval) {
    free(buf);
    return;
  }
  int stdoutlen = strlen(buf);
#else
  const char *templ = "/tmp/bilge-XXXXXX";
  char *namebuf = malloc(strlen(templ)+1);
  strcpy(namebuf, templ);
  int out = mkstemp(namebuf);
  unlink(namebuf);
  free(namebuf);

  pid_t new_pid = fork();
  if (new_pid == 0) {
    char **args = malloc(3*sizeof(char *));
    close(1); dup(out);
    close(2);
    open("/dev/null", O_WRONLY);
    args[0] = "git";
    args[1] = "ls-files";
    args[2] = 0;
    execvp("git", args);
    exit(0);
  }
  int status = 0;
  if (waitpid(new_pid, &status, 0) != new_pid) {
    printf("Unable to exec git ls-files\n");
    return; // fixme should exit
  }
  if (WEXITSTATUS(status)) {
    printf("Unable to run git ls-files successfully %d\n", WEXITSTATUS(status));
    //    return 0;
  }
  off_t stdoutlen = lseek(out, 0, SEEK_END);
  lseek(out, 0, SEEK_SET);
  char *buf = malloc(stdoutlen);
  if (read(out, buf, stdoutlen) != stdoutlen) {
    printf("Error reading output of git ls-files\n");
    free(buf);
    return; // fixme should exit
  }
#endif

  int last_start = 0;
  for (int i=0;i<stdoutlen;i++) {
    if (buf[i] == '\n') {
      buf[i] = 0;
      char *path = absolute_path(root, buf + last_start);
      struct target *t = create_target(all, path);
      free(path);
      assert(t);
      t->is_in_git = true;
      // Now check if this file is in a subdirectory...
      for (int j=i-1;j>last_start;j--) {
        if (buf[j] == '/') {
          buf[j] = 0;
          char *path = absolute_path(root, buf + last_start);
          struct target *t = create_target(all, path);
          free(path);
          assert(t);
          t->is_in_git = true;
        }
      }
      last_start = i+1;
    }
  }
  free(buf);
}


void git_add(const char *path) {
  const char **args = malloc(6*sizeof(char *));
  args[0] = "git";
  args[1] = "add";
  args[2] = "-f";
  args[3] = "--";
  args[4] = path;
  args[5] = 0;

#ifdef _WIN32
  int retval = spawnvp(P_WAIT, "git", (char **)args);
#else
  pid_t new_pid = fork();
  if (new_pid == 0) {
    close(0);
    execvp("git", (char **)args);
    error(1, errno, "running git");
  }
  int status = 0;
  if (waitpid(new_pid, &status, 0) != new_pid) {
    printf("Unable to exec git add -- %s\n", path);
    return; // fixme should exit
  }
  int retval = WEXITSTATUS(status);
#endif
  if (retval) {
    printf("Unable to run git add -- %s successfully %d\n", path, retval);
  }
}
