#include <stdio.h>
extern void uart0_init(void);
extern void semihost_exit(int code) __attribute__((noreturn));

int main(void) {
  uart0_init();
  printf("hello, an500\n");
  semihost_exit(0);
}
