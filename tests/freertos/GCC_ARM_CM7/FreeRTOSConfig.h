#define configCPU_CLOCK_HZ                      (25000000UL)
#define configTICK_RATE_HZ                      (1000)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (0x50)
#define configMAX_PRIORITIES                    (11)
#define configMINIMAL_STACK_SIZE                (128)
#define configSUPPORT_STATIC_ALLOCATION         (1)
#define configSUPPORT_DYNAMIC_ALLOCATION        (0)
#define configTICK_TYPE_WIDTH_IN_BITS           (TICK_TYPE_WIDTH_32_BITS)
#define configUSE_PREEMPTION                    (1)
#define configUSE_IDLE_HOOK                     (0)
#define configUSE_TICK_HOOK                     (0)
#define configUSE_TIMERS                        (1)
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                (10)
#define configTIMER_TASK_STACK_DEPTH            (128)
#define configUSE_MUTEXES                       (1)
#define configUSE_RECURSIVE_MUTEXES             (0)
#define configUSE_COUNTING_SEMAPHORES           (0)
#define configUSE_QUEUE_SETS                    (0)
#define configMAX_TASK_NAME_LEN                 (16)
#define configUSE_TRACE_FACILITY                (0)
#define configCHECK_FOR_STACK_OVERFLOW          (0)
#define configUSE_STATS_FORMATTING_FUNCTIONS    (0)
#define configUSE_PORT_OPTIMISED_TASK_SELECTION (1)
#define configUSE_TICKLESS_IDLE                 (0)
#define configASSERT(x)                                                                            \
  if ((x) == 0) {                                                                                  \
    taskDISABLE_INTERRUPTS();                                                                      \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

#define INCLUDE_vTaskDelay                  (1)
#define INCLUDE_vTaskDelete                 (1)
#define INCLUDE_xTaskGetCurrentTaskHandle   (1)
#define INCLUDE_uxTaskGetStackHighWaterMark (0)
#define INCLUDE_xTaskGetIdleTaskHandle      (0)

#define vPortSVCHandler                     SVC_Handler
#define xPortPendSVHandler                  PendSV_Handler
#define xPortSysTickHandler                 SysTick_Handler
