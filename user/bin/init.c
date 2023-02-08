#include <ulib.h>

int main(char *argument) {
  printf("Hello from userspace!. Type some stuff (type nothing to continue)\n");

  while (1) {
    printf("$ ");
    char buf[256];
    long len = readline(buf, sizeof(buf));

    printf("you typed %d chars: '%s'\n", len, buf);
    for (int i = 0; i < len; i++) {
      printf("0x%x ", buf[i]);
    }
    printf("\n");
    if (len == 0)
      break;
  }
  printf("Goodbye!\n");

  return 0;
}