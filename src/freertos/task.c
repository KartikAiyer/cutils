#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <cutils/task.h>
#include <string.h>

static void task_wrapper(void *arg) {
  task_t *task = (task_t *)arg;
  task->func(task->ctx);
  vTaskSuspend(NULL);  // wait for task_destroy_static to delete us
}

task_t *task_new_static(task_create_params_t *params) {
  if (params) {
    task_t *task = params->task;
    task->func = params->func;
    task->ctx = params->ctx;
    task->task = xTaskCreateStatic(task_wrapper,
                                   params->label,
                                   params->stack_size / sizeof(StackType_t),
                                   task,
                                   params->priority,
                                   params->stack,
                                   &task->tcb);
    return task;
  }
  return 0;
}

/**
 * FreeRTOS auto start the task on creation. This is a nop
 */
bool task_start(task_t *task) {
  (void)task;
  return true;
}

void task_destroy_static(task_t *task) {
  if (task && task->task) {
    vTaskDelete(task->task);
    task->task = NULL;
  }
}

uint64_t task_get_ticks(void) {
  /* xTaskGetTickCount() returns TickType_t (uint32_t with our 32-bit tick
   * config). Widening to uint64_t is exact but the underlying counter rolls
   * over every ~49.7 days at 1 kHz; a 64-bit overflow-tracking wrapper can be
   * added later if a test needs longer uptimes. */
  return (uint64_t)xTaskGetTickCount();
}

void task_sleep(uint32_t ms) {
  /* pdMS_TO_TICKS(0) == 0; vTaskDelay(0) yields to equal-priority tasks rather
   * than sleeping, which matches the intent of a zero-timeout 'sleep'. */
  vTaskDelay((ms == WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(ms));
}

void task_get_current_name(char *name, size_t string_length) {
  if (name && string_length) {
    const char *pcName = pcTaskGetName(xTaskGetCurrentTaskHandle());
    if (pcName) {
      strncpy(name, pcName, string_length - 1);
    } else {
      strncpy(name, "unknown", string_length - 1);
    }
    name[string_length - 1] = '\0';
  }
}

// Implement Idle Task and Timer Task memory hooks which are needed when only static allocation is
// used
#if (configSUPPORT_STATIC_ALLOCATION == 1)
void vApplicationGetIdleTaskMemory(StaticTask_t **idle_task_tcb,
                                   StackType_t **idle_task_stack,
                                   configSTACK_DEPTH_TYPE *idle_task_stack_size) {
  static StaticTask_t s_idleTask;
  static alignas(CUTILS_TASK_STACK_ALIGN) StackType_t s_idleTaskStack[configMINIMAL_STACK_SIZE];

  *idle_task_tcb = &s_idleTask;
  *idle_task_stack = s_idleTaskStack;
  *idle_task_stack_size = configMINIMAL_STACK_SIZE;
}
#endif

#if (configUSE_TIMERS == 1)
void vApplicationGetTimerTaskMemory(StaticTask_t **timer_task_tcb,
                                    StackType_t **timer_task_stack,
                                    configSTACK_DEPTH_TYPE *timer_task_stack_size) {
  static StaticTask_t s_timerTask;
  static alignas(CUTILS_TASK_STACK_ALIGN)
      StackType_t s_timerTaskStack[configTIMER_TASK_STACK_DEPTH];

  *timer_task_tcb = &s_timerTask;
  *timer_task_stack = s_timerTaskStack;
  *timer_task_stack_size = configTIMER_TASK_STACK_DEPTH;
}
#endif
