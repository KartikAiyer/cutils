#include <FreeRTOSConfig.h>
#include <stdint.h>
#include <sys/stat.h>

typedef struct {
  volatile uint32_t DATA;
  volatile uint32_t STATE;
  volatile uint32_t CTRL;
  volatile uint32_t INTSTATUS;
  volatile uint32_t BAUDDIV;
} cmsdk_uart_t;

#define UART0           ((cmsdk_uart_t *)0x40004000UL)

#define CTRL_TX_EN      (1u << 0)
#define CTRL_RX_EN      (1u << 1)
#define STATE_TXFULL    (1u << 0)
#define STATE_RXFULL    (1u << 1)
#define STATE_TXOVERRUN (1u << 2)
#define STATE_RXOVERRUN (1u << 3)

void uart0_init(void) {
  UART0->BAUDDIV = configCPU_CLOCK_HZ / 115200;
  UART0->CTRL = CTRL_TX_EN;
  // Not setting up RX since its not needed for the tests
  UART0->STATE = STATE_TXOVERRUN | STATE_RXOVERRUN;
}

static void uart_putc(char c) {
  if (c == '\n') {
    while (UART0->STATE & STATE_TXFULL)
      ;
    UART0->DATA = (uint32_t)'\r';
  }
  while (UART0->STATE & STATE_TXFULL)
    ;
  UART0->DATA = (uint32_t)c;
}

int _write(int file, char *ptr, int len) {
  if (file != 1 && file != 2)
    return -1;

  for (int i = 0; i < len; i++) {
    uart_putc(ptr[i]);
  }
  return len;
}

__attribute__((noreturn)) void semihost_exit(int code) {
  register uint32_t r0 __asm("r0") = 0x18;
  register uint32_t r1 __asm("r1") = (code == 0) ? 0x20026 : 0x20028;

  __asm volatile("bkpt 0xAB" : : "r"(r0), "r"(r1) : "memory");
  while (1)
    ;
}

int _close(int fd) {
  (void)fd;
  return -1;
}

int _lseek(int fd, int offset, int whence) {
  (void)fd;
  (void)offset;
  (void)whence;
  return -1;
}

int _read(int fd, char *ptr, int len) {
  (void)fd;
  (void)ptr;
  (void)len;
  return -1;
}

int _isatty(int fd) {
  (void)fd;
  return 1;
}

int _fstat(int fd, struct stat *st) {
  (void)fd;
  st->st_mode = 0x2000;
  return 0;
}

void *_sbrk(int incr) {
  (void)incr;
  return (void *)-1;
}
