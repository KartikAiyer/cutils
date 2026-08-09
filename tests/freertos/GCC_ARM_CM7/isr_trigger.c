#include <isr_trigger.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct _state_t {
  volatile isr_handler handler;
  void *volatile data;
  volatile bool ran_in_handler_mode;
} state_t;

static state_t s_state = {0};

#define NVIC_ISER ((volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER ((volatile uint32_t *)0xE000E180UL)
#define NVIC_ISPR ((volatile uint32_t *)0xE000E200UL)
#define NVIC_ICPR ((volatile uint32_t *)0xE000E280UL)
#define NVIC_IPR  ((volatile uint8_t *)0xE000E400UL)

void isr_arm(uint8_t priority, isr_handler handler, void *data) {
  // Clear pending bits
  NVIC_ICPR[0] = 1u;
  s_state.ran_in_handler_mode = false;
  s_state.handler = handler;
  s_state.data = data;
  NVIC_IPR[0] = priority;
  NVIC_ISER[0] = 1u;
}

void isr_disarm(void) {
  NVIC_ICER[0] = 1u;
  s_state.handler = NULL;
}
void isr_fire(void) {
  NVIC_ISPR[0] = 1u;
  __asm volatile("dsb 0xF" ::: "memory");
  __asm volatile("isb 0xF" ::: "memory");
}

void Interrupt0_Handler(void) {
  uint32_t ipsr;
  __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
  s_state.ran_in_handler_mode = (ipsr != 0);
  isr_handler handler = s_state.handler;
  void *data = s_state.data;
  if (handler)
    handler(data);
}

bool isr_did_run_in_handler(void) { return s_state.ran_in_handler_mode; }
