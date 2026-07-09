#include <stdint.h>
#include <string.h>

extern unsigned long _estack;
extern unsigned long __copy_table_start__[];
extern unsigned long __copy_table_end__[];
extern unsigned long __zero_table_start__[];
extern unsigned long __zero_table_end__[];

void Reset_Handler(void) __attribute__((noreturn));

extern int main(void);

void Default_Handler(void) {
  while (1)
    ;
}
void HardFault_Handler(void) {
  while (1)
    ;
}

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));

void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

void Interrupt0_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt1_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt2_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt3_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt4_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt5_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt6_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt7_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt8_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Interrupt9_Handler(void) __attribute__((weak, alias("Default_Handler")));

typedef void (*vector_table_t)(void);

const vector_table_t vectors[48]
    __attribute__((used, section(".vectors"), aligned(256))) = {(vector_table_t)&_estack,
                                                                Reset_Handler,
                                                                NMI_Handler,
                                                                HardFault_Handler,
                                                                MemManage_Handler,
                                                                BusFault_Handler,
                                                                UsageFault_Handler,
                                                                0,
                                                                0,
                                                                0,
                                                                0,
                                                                SVC_Handler,
                                                                DebugMon_Handler,
                                                                0,
                                                                PendSV_Handler,
                                                                SysTick_Handler,
                                                                Interrupt0_Handler,
                                                                Interrupt1_Handler,
                                                                Interrupt2_Handler,
                                                                Interrupt3_Handler,
                                                                Interrupt4_Handler,
                                                                Interrupt5_Handler,
                                                                Interrupt6_Handler,
                                                                Interrupt7_Handler,
                                                                Interrupt8_Handler,
                                                                Interrupt9_Handler};

typedef struct {
  void *source;
  void *destination;
  size_t length;
} cpy_tbl_t;

typedef struct {
  void *destination;
  size_t length;
} zero_tbl_t;

__attribute__((noreturn)) void Reset_Handler(void) {
  for (cpy_tbl_t *cur = (cpy_tbl_t *)__copy_table_start__; cur != (cpy_tbl_t *)__copy_table_end__;
       cur++) {
    memcpy(cur->destination, cur->source, cur->length * sizeof(uint32_t));
  }
  for (zero_tbl_t *cur = (zero_tbl_t *)__zero_table_start__;
       cur != (zero_tbl_t *)__zero_table_end__;
       cur++) {
    memset(cur->destination, 0, cur->length * sizeof(uint32_t));
  }
  main();
  while (1)
    ;
}
