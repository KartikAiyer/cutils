#include <FreeRTOSConfig.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/unistd.h>

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

/**
 * There is a small heap just for the tests and the use of rand
 */
void *_sbrk(int incr) {
  extern char __end__;
  extern char __HeapLimit;

  static char *heap_ptr = NULL;
  char *prev;

  if (heap_ptr == NULL)
    heap_ptr = &__end__;

  prev = heap_ptr;
  if (heap_ptr + incr > &__HeapLimit) {
    return (void *)-1;
  }
  heap_ptr += incr;
  return (void *)prev;
}

__attribute__((noreturn)) void _exit(int status) { semihost_exit(status); }

pid_t _getpid(void) { return (pid_t)1; }

int _kill(pid_t pid, int sig) { return -1; }

int _gettimeofday(struct timeval *tv, void *tz) {
  (void)tz;
  if (tv) {
    tv->tv_sec = 0;
    tv->tv_usec = 0;
    return 0;
  }
  return -1;
}

uint64_t __atomic_fetch_add_8(void *mem, uint64_t val, int model) {
  (void)model;
  uint64_t *p = (uint64_t *)mem;
  uint32_t primask;
  __asm volatile("mrs %0, primask" : "=r"(primask));
  __asm volatile("cpsid i" : : : "memory");
  uint64_t old = *p;
  *p = old + val;
  __asm volatile("msr primask, %0" : : "r"(primask) : "memory");
  return old;
}
uint64_t __atomic_load_8(const void *mem, int model) {
  (void)model;
  const uint64_t *p = (const uint64_t *)mem;
  uint64_t value;
  uint32_t primask;
  __asm volatile("mrs %0, primask" : "=r"(primask));
  __asm volatile("cpsid i" : : : "memory");
  value = *p;
  __asm volatile("msr primask, %0" : : "r"(primask) : "memory");
  return value;
}

void __atomic_store_8(void *mem, uint64_t val, int model) {
  (void)model;
  uint64_t *p = (uint64_t *)mem;
  uint32_t primask;
  __asm volatile("mrs %0, primask" : "=r"(primask));
  __asm volatile("cpsid i" : : : "memory");
  *p = val;
  __asm volatile("msr primask, %0" : : "r"(primask) : "memory");
}
