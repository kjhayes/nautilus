// This is an implementation of a *very* simple shell.

#include <ulib.h>

int main() {
  while (1) {
    printf("shell$ ");
    char buf[256];
    long len = readline(buf, sizeof(buf));
    if (len == 0)
      continue;

    if (strcmp(buf, "exit") == 0) {
      return 0;
    }
    pid_t pid = spawn(buf, "argument");
    if (pid == -1) {
      printf("Unknown command: %s\n", buf);
      continue;
    }
    wait(pid);
  }

  return 0;
}
