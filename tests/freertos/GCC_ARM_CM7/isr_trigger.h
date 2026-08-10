#pragma once

#include <FreeRTOSConfig.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Default NVIC priority for the trigger interrupt.
 *
 * Cortex-M priorities are inverted: numerically lower means more urgent. FreeRTOS protects its
 * *FromISR API by raising BASEPRI to configMAX_SYSCALL_INTERRUPT_PRIORITY, which cannot mask
 * anything numerically below itself — so an ISR that calls those APIs must sit at a priority
 * numerically >= that threshold. vPortValidateInterruptPriority() enforces it with configASSERT,
 * which on this target is an interrupts-off while(1): a violation shows up as a ctest timeout with
 * no diagnostic, not as a failed assertion.
 *
 * Equality satisfies the check, so the threshold itself is the safest self-documenting choice.
 * Note the NVIC reset default is 0 — maximum urgency — so the priority must always be set
 * explicitly before enabling the interrupt.
 */
#define ISR_TRIGGER_PRIORITY ((uint8_t)configMAX_SYSCALL_INTERRUPT_PRIORITY)

typedef void (*isr_handler)(void *data);

void isr_arm(uint8_t priority, isr_handler handler, void *data);
void isr_disarm(void);
void isr_fire(void);
bool isr_did_run_in_handler(void);
