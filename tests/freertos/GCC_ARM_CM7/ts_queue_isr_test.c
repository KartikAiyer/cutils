/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2026> <Kartik Aiyer>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <isr_trigger.h>
#include <embUnit/embUnit.h>
#include <cutils/ts_queue.h>
#include <cutils/task.h>
#include <cutils/signal.h>
#include <string.h>

typedef struct {
  // Smoke Test Data
  volatile bool ran;
  void *volatile seen_data;
  // General test data
  ts_queue_t *queue;
  task_t *task;
  uint32_t happy_path_return_value;
  bool enqueue_result;
  bool dequeue_result;
  signal_t flag;
} test_state_t;

static test_state_t s_state;

static uint32_t s_sentinel = 0xC0FFEEu;

static void smoke_isr(void *data) {
  s_state.ran = true;
  s_state.seen_data = data;
}

static void isr_setUp(void) {
  memset(&s_state, 0, sizeof(s_state));
  signal_new(&s_state.flag);
}
static void isr_tearDown(void) {
  isr_disarm();
  if (s_state.task) {
    task_destroy_static(s_state.task);
    s_state.task = 0;
  }
  if (s_state.queue) {
    ts_queue_destroy(s_state.queue);
    s_state.queue = 0;
  }
  signal_free(&s_state.flag);
}

static void trigger_fires_and_dispatches(void) {
  isr_arm(ISR_TRIGGER_PRIORITY, smoke_isr, &s_sentinel);
  TEST_ASSERT(!isr_did_run_in_handler());
  isr_fire();
  TEST_ASSERT(isr_did_run_in_handler());
  TEST_ASSERT(s_state.ran);
  TEST_ASSERT(s_state.seen_data == &s_sentinel);
}

static void disarm_prevents_dispatch(void) {
  isr_arm(ISR_TRIGGER_PRIORITY, smoke_isr, &s_sentinel);
  isr_fire();
  TEST_ASSERT(s_state.ran);

  s_state.ran = false;
  isr_disarm();
  isr_fire();
  TEST_ASSERT(!s_state.ran);
}

TS_QUEUE_STORE_DECL(test_queue, 4);
TS_QUEUE_STORE_DEF(test_queue);

static void happy_path_queue_setup(void) {
  ts_queue_create_params_t params = {0};
  TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, test_queue);
  s_state.queue = ts_queue_init(&params);
}

static void happy_isr_f(void *data) {
  (void)data;
  bool woken = false;
  s_state.enqueue_result = ts_queue_enqueue_from_isr(s_state.queue, (void *)0xDEADBEEF, &woken);
}

static void happy_path_test_task_f(void *arg1) {
  (void)arg1;
  happy_path_queue_setup();
  isr_arm(ISR_TRIGGER_PRIORITY, happy_isr_f, NULL);
  isr_fire();
  void *item = 0;
  s_state.dequeue_result = ts_queue_dequeue(s_state.queue, &item, NO_SLEEP);
  s_state.happy_path_return_value = (uint32_t)item;
  signal_send(&s_state.flag);
}

TASK_STATIC_STORE_DECL(test_task, 4 * 1024);
TASK_STATIC_STORE_DEF(test_task);

static void happy_path_test(void) {
  // Create a task
  task_create_params_t params = {0};

  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 test_task,
                                 "queue_isr_task",
                                 CUTILS_TASK_PRIORITY_MEDIUM,
                                 happy_path_test_task_f,
                                 NULL);

  s_state.task = task_new_static(&params);
  TEST_ASSERT_MESSAGE(s_state.task, "Failed to create Task to run queue isr tests");
  task_start(s_state.task);

  TEST_ASSERT_MESSAGE(signal_wait_timed(&s_state.flag, 100),
                      "Could not get task complete signal within timeout");

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "The ISR did not run");
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed To create Queue");
  TEST_ASSERT_MESSAGE(s_state.enqueue_result, "Failed to enqueue item onto queue");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Failed to dequeue from the task");
  TEST_ASSERT_MESSAGE(s_state.happy_path_return_value == 0xDEADBEEF,
                      "Did not get the expected value after dequeue");
}

TestRef queue_ts_queue_isr_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("ISR trigger fires and dispatches", trigger_fires_and_dispatches),
      new_TestFixture("Disarm prevents dispatch", disarm_prevents_dispatch),
      new_TestFixture("Enqueue item from ISR Happy path", happy_path_test)};
  EMB_UNIT_TESTCALLER(
      queue_ts_queue_isr_tests, "queue_ts_queue_isr_test", isr_setUp, isr_tearDown, fixtures);
  return (TestRef)&queue_ts_queue_isr_tests;
}
