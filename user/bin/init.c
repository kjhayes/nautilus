#include <ulib.h>


struct node {
  int value;
  struct node *next;
};



int main(char *argument) {

  struct node *root = NULL;

  for (int i = 0; i < 10; i++) {
    struct node *n = calloc(1, sizeof(struct node));
    n->value = i;
    n->next = root;
    root = n;
    printf("made node %x\n", n);
  }
  // volatile long *data = valloc(12);
  // printf("Hello, world. %x\n", *data);
  // return 0;
  // while (1) {
  //   pid_t pid = spawn("hello", "argument");
  //   wait(pid);
  // }
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