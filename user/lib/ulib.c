

void start(int argc, char **argv) {
  //
  while (1) {
    // do a systemcall for testing
    asm("int $0x80");
  }
}