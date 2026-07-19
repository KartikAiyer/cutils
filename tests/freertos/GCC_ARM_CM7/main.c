#include <FreeRTOS.h>
#include <cutils/task.h>
#include <embUnit/embUnit.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void uart0_init(void);
extern void semihost_exit(int code) __attribute__((noreturn));

/* Stringify the suite accessor selected at compile time
 * (-DCUTILS_EMBTEST_SUITE=<fn>) for the per-suite runner's label. */
#define EMBTEST_STR_(s) #s
#define EMBTEST_STR(s) EMBTEST_STR_(s)

#define RUNNER_STACK_SIZE 2048

static void run_suite(const char *name, TestRef (*get_tests)(void)) {
  printf("\n[%s] ", name);
  TestRunner_runTest(get_tests());
}

#ifndef CUTILS_EMBTEST_SUITE
/* ---------------------------------------------------------------------------
 * Aggregate runner: every suite in one QEMU boot. Built when
 * CUTILS_EMBTEST_SUITE is undefined (the embtest_runner target, opt-in via
 * CUTILS_FREERTOS_AGGREGATE_RUNNER). Kept for fast "run everything" smoke
 * tests — it boots QEMU once instead of N times.
 * ------------------------------------------------------------------------- */

/* Forward declarations — every suite's TestRef accessor */
extern TestRef klist_get_tests(void);
extern TestRef mem_ring_buffer_get_tests(void);
extern TestRef os_mutex_get_tests(void);
extern TestRef os_event_flag_get_tests(void);
extern TestRef os_task_get_tests(void);
extern TestRef queue_free_list_get_tests(void);
extern TestRef queue_kqueue_get_tests(void);
extern TestRef queue_ts_queue_simple_get_tests(void);
extern TestRef queue_ts_queue_get_tests(void);
extern TestRef pool_get_tests(void);
extern TestRef notifier_get_tests(void);
extern TestRef notifier_static_store_get_tests(void);
extern TestRef accumulator_get_tests(void);
extern TestRef dispatch_queue_get_tests(void);
extern TestRef asyncio_get_tests(void);
extern TestRef state_event_loop_get_tests(void);

static void runner_task(void *arg) {
  (void)arg;
  TestRunner_start();
  run_suite("klist", klist_get_tests);
  run_suite("mem_ring_buffer", mem_ring_buffer_get_tests);
  run_suite("os_mutex", os_mutex_get_tests);
  run_suite("os_event_flag", os_event_flag_get_tests);
  run_suite("os_task", os_task_get_tests);
  run_suite("queue_free_list", queue_free_list_get_tests);
  run_suite("queue_kqueue", queue_kqueue_get_tests);
  run_suite("queue_ts_queue_simple", queue_ts_queue_simple_get_tests);
  run_suite("queue_ts_queue", queue_ts_queue_get_tests);
  run_suite("pool", pool_get_tests);
  run_suite("notifier", notifier_get_tests);
  run_suite("notifier_static_store", notifier_static_store_get_tests);
  run_suite("accumulator", accumulator_get_tests);
  run_suite("dispatch_queue", dispatch_queue_get_tests);
  run_suite("asyncio", asyncio_get_tests);
  run_suite("state_event_loop", state_event_loop_get_tests);
  TestRunner_end();
  semihost_exit(TestRunner_failureCount() ? 1 : 0);
}
#else
/* ---------------------------------------------------------------------------
 * Per-suite runner: exactly one suite, selected at compile time via
 * -DCUTILS_EMBTEST_SUITE=<accessor>. Each embtest_<suite> ELF boots QEMU,
 * runs that one suite, and semihost_exits with pass/fail — giving ctest
 * first-class per-suite granularity instead of one aggregate result.
 * ------------------------------------------------------------------------- */

/* The accessor is injected by the build as a function name token. */
extern TestRef CUTILS_EMBTEST_SUITE(void);

static void runner_task(void *arg) {
  (void)arg;
  TestRunner_start();
  run_suite(EMBTEST_STR(CUTILS_EMBTEST_SUITE), CUTILS_EMBTEST_SUITE);
  TestRunner_end();
  semihost_exit(TestRunner_failureCount() ? 1 : 0);
}
#endif

TASK_STATIC_STORE_DECL(runner, RUNNER_STACK_SIZE);
TASK_STATIC_STORE_DEF(runner);

int main(void) {
  uart0_init();
  setvbuf(stdout, NULL, _IONBF, 0);

  task_create_params_t params;
  TASK_STATIC_INIT_CREATE_PARAMS(
      params, runner, "runner", CUTILS_TASK_PRIORITY_MEDIUM, runner_task, NULL);
  task_new_static(&params);

  vTaskStartScheduler();
  for (;;)
    ;
}
