#include <ulib.h>

int main() {

  void *kernel_address = (void *)0x1000;
  printf("I'm about to print memory from the kernel.\n");
  // Nautilus doesn't protect against the user doing sneaky stuff like this!
  write(kernel_address, 0x1000);

  // Why did that work?
  printf("I did it!\n");

  return 0;
}
