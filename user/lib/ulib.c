#include <ulib.h>

// Define an assembly function which invokes the `int 0x80` instruction to enter
// the kernel using the classic systemcall interface. The ABI in the userspace
// built here is the same as the C function call ABI (rdi,rsi,etc... with rax
// containing the return address). This just simplifies things a bit :)
asm(".global __syscall\n"
    "__syscall:\n"
    "  int $0x80\n"
    "  ret");

// Tell the linker to put this symbol at the start of the flat binary. (the
// only symbol in .init is start). This function exists to emulate a normal
// '__libc_start' function in a glibc-based user application. In a more
// complicated environment, it would run initializer functions and setup
// things like the memory allocator. In our userspace, it just calls off to
// main then calls exit when main returns.
__attribute__((section(".init"), noinline)) void start(int argc, char **argv) {
  //
  while (1) {
    // do a systemcall for testing
    asm("int $0x80");
  }

  exit();
}

void exit(void) {}