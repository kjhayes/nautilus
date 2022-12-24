#include <ulib.h>

int main() {
  // just make a systemcall
  return syscall(0, 1, 2, 3);
}
