#include <ulib.h>

int main(char *argument) {
  console_write("Hello, world: ");
  console_write(argument);
  console_write("\n");
  return 0;
}