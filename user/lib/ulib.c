#include <ulib.h>

extern void main(const char *cmd, const char *arg);

// This function is the entrypoint to the userspace binary
__attribute__((section(".init"), noinline)) void start(const char *cmd,
                                                       const char *arg) {
  main(cmd, arg);
  exit();
}

void exit(void) { syscall0(SYSCALL_EXIT); }

int console_write(const char *message) {
  return syscall2(SYSCALL_WRITE, message, strlen(message));
}

// ----------------------------------------------------

long strlen(const char *s) {
  long len = 0;
  for (len = 0; s[len]; len++) {
    // ...
  }
  return len;
}

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

static void printint(int xx, int base, int sgn) {
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint32_t x;

  neg = 0;
  if (sgn && xx < 0) {
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  if (neg)
    buf[i++] = '-';

  while (--i >= 0)
    putc(buf[i]);
}

// A simple printf. Only understands %d, %x, %p, %s.
void printf(char *fmt, ...) {
  va_list ap;
  char *s;
  int c, i, state;
  va_start(ap, fmt);

  state = 0;
  for (i = 0; fmt[i]; i++) {
    c = fmt[i] & 0xff;
    if (state == 0) {
      if (c == '%') {
        state = '%';
      } else {
        putc(c);
      }
    } else if (state == '%') {
      if (c == 'd') {
        printint(va_arg(ap, int), 10, 1);
      } else if (c == 'x' || c == 'p') {
        printint(va_arg(ap, int), 16, 0);
      } else if (c == 's') {
        s = va_arg(ap, char *);
        if (s == 0)
          s = "(null)";
        while (*s != 0) {
          putc(*s);
          s++;
        }
      } else if (c == 'c') {
        putc(va_arg(ap, uint32_t));
      } else if (c == '%') {
        putc(c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        putc('%');
        putc(c);
      }
      state = 0;
    }
  }
}

// Simply write a single byte to the console
void putc(char c) { syscall2(SYSCALL_WRITE, &c, 1); }