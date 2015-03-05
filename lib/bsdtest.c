#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <signal.h>

void die(const char *str) {
  printf("die: %s with errno %s\n", str, strerror(errno));
  exit(1);
}

int main(int argc, char **argv) {
  int pid = fork();
  setpgid(pid, pid); /* set the group id */
  if (pid == -1) die("bad fork");
  if (pid == 0) {
    if (ptrace(PT_TRACE_ME, 0, 0, 0) != 0) die("tramce");
    execvp(argv[1], argv+1);
  }
  pid_t child = waitpid(pid, 0, 0);
  if (child < 0) die("foo");
  if (child != pid) die("different kid");
  ptrace(PT_TO_SCE, pid, (caddr_t)1, 0);

  while (1) {
    int status;
    pid_t child = waitpid(-pid, &status, 0);
    if (child < 0) die("waitfailed");
    struct reg regs;
    errno = 0;
    if (ptrace(PT_GETREGS, pid, (caddr_t) &regs, 0) < 0)
      die("regs");
    printf("pid %d system call # %ld status %d\n",
	   child, regs.r_rax, status);
    if (ptrace(PT_FOLLOW_FORK, child, 0, 0) != 0)
      die("followfork");
    if (ptrace(PT_TO_SCX, child, (caddr_t)1, 0) < 0) die("scx");
    if (waitpid(child, 0, 0) < 0) die("bbb");
    if (ptrace(PT_TO_SCE, child, (caddr_t)1, 0) < 0) die("scx");
  }
  while (ptrace(PT_TO_SCE, pid, (caddr_t)1, 0) == 0) {
    if (waitpid(pid, 0, 0) < 0) die("aaa");
    struct reg regs;
    errno = 0;
    if (ptrace(PT_GETREGS, pid, (caddr_t) &regs, 0) < 0)
      die("regs");
    printf("system call # %ld\n", regs.r_rax);
    if (ptrace(PT_TO_SCX, pid, (caddr_t)1, 0) < 0) die("scx");
    if (waitpid(pid, 0, 0) < 0) die("bbb");
  }
  if (wait(0) < 0) die("ugh");
  printf("all done!\n");
  return 0;
}
