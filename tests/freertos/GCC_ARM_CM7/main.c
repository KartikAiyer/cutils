#include <FreeRTOS.h>
#include <cutils/task.h>
#include <embUnit/embUnit.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void uart0_init(void);
extern void semihost_exit(int code) __attribute__((noreturn));

/* ---------------------------------------------------------------------------
 * One runner ELF per suite: the suite is selected at compile time via
 * -DCUTILS_EMBTEST_SUITE=<accessor>. Each embtest_<suite> ELF boots QEMU from
 * reset, runs that one suite, and semihost_exits with pass/fail, giving ctest
 * per-suite granularity, per-suite timeouts, and no global hardware state
 * carried between suites.
 * ------------------------------------------------------------------------- */

#ifndef CUTILS_EMBTEST_SUITE
#error "CUTILS_EMBTEST_SUITE must name the suite's TestRef accessor. Register \
the target with freertos_add_embunit_test(NAME <suite> SUITE_FN <accessor>) \
rather than adding an executable by hand."
#endif

/* Stringify the suite accessor selected at compile time
 * (-DCUTILS_EMBTEST_SUITE=<fn>) for the runner's label. */
#define EMBTEST_STR_(s)   #s
#define EMBTEST_STR(s)    EMBTEST_STR_(s)

#define RUNNER_STACK_SIZE 2048

static void run_suite(const char *name, TestRef (*get_tests)(void)) {
  printf("\n[%s] ", name);
  TestRunner_runTest(get_tests());
}

/* The accessor is injected by the build as a function name token. */
extern TestRef CUTILS_EMBTEST_SUITE(void);

static void runner_task(void *arg) {
  (void)arg;
  TestRunner_start();
  run_suite(EMBTEST_STR(CUTILS_EMBTEST_SUITE), CUTILS_EMBTEST_SUITE);
  TestRunner_end();
  semihost_exit(TestRunner_failureCount() ? 1 : 0);
}

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
