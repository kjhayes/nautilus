#include <ulib.h>

int main(char *argument) {
  printf("Hello, world\n");
  return 0;
  while (1) {
    pid_t pid = spawn("hello", "argument");
    wait(pid);
  }
  printf("Hello from userspace!. Type some stuff (type nothing to continue)\n");

  while (1) {
    printf("shell$ ");
    char buf[256];
    long len = readline(buf, sizeof(buf));
    if (len == 0)
      break;

    pid_t pid = spawn(buf, "argument");
    wait(pid);
  }
  printf("Goodbye!\n");

  return 0;
}