#include <ulib.h>


int main(char *argument) {
  printf("Hello from userspace!\n");

  while (1) {
    printf("[init] starting shell (/sh)\n");
    pid_t pid = spawn("/sh", "");
    wait(pid);
  }
  return 0;
}