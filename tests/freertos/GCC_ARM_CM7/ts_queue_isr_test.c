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
  ts_queue_t *second_queue;
  task_t *task;
  volatile uint32_t single_dequeued_return_value;
  volatile bool enqueue_result;
  volatile bool dequeue_result;
  signal_t flag;
  volatile bool woken;
} test_state_t;

static test_state_t s_state;

static uint32_t s_sentinel = 0xC0FFEEu;

#define QUEUE_SIZE (4)
TS_QUEUE_STORE_DECL(test_queue, QUEUE_SIZE);
TS_QUEUE_STORE_DEF(test_queue);

TS_QUEUE_STORE_DECL(second_test_queue, QUEUE_SIZE);
TS_QUEUE_STORE_DEF(second_test_queue);

static void queue_setup(void) {
  ts_queue_create_params_t params;
  TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, test_queue);
  s_state.queue = ts_queue_init(&params);
  TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, second_test_queue);
  s_state.second_queue = ts_queue_init(&params);
}

static void smoke_isr(void *data) {
  s_state.ran = true;
  s_state.seen_data = data;
}

static void isr_setUp(void) {
  memset(&s_state, 0, sizeof(s_state));
  signal_new(&s_state.flag);
  queue_setup();
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
  if (s_state.second_queue) {
    ts_queue_destroy(s_state.second_queue);
    s_state.second_queue = 0;
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

static void happy_isr_f(void *data) {
  (void)data;
  bool woken = false;
  s_state.enqueue_result = ts_queue_enqueue_from_isr(s_state.queue, (void *)0xDEADBEEF, &woken);
  s_state.woken = woken;
}

static void happy_path_test_task_f(void *arg1) {
  (void)arg1;
  isr_arm(ISR_TRIGGER_PRIORITY, happy_isr_f, NULL);
  isr_fire();
  void *item = 0;
  s_state.dequeue_result = ts_queue_dequeue(s_state.queue, &item, NO_SLEEP);
  s_state.single_dequeued_return_value = (uint32_t)item;
  signal_send(&s_state.flag);
}

TASK_STATIC_STORE_DECL(test_task, 4 * 1024);
TASK_STATIC_STORE_DEF(test_task);

#define TEST_TASK_CREATE(fn, priority)                                                             \
  do {                                                                                             \
    task_create_params_t params = {0};                                                             \
    TASK_STATIC_INIT_CREATE_PARAMS(params, test_task, "queue_isr_task", priority, fn, NULL);       \
    s_state.task = task_new_static(&params);                                                       \
  } while (0)

static void happy_path_test(void) {
  // Create a task
  TEST_TASK_CREATE(happy_path_test_task_f, CUTILS_TASK_PRIORITY_MEDIUM);

  TEST_ASSERT_MESSAGE(s_state.task, "Failed to create Task to run queue isr tests");
  task_start(s_state.task);

  TEST_ASSERT_MESSAGE(signal_wait_timed(&s_state.flag, 100),
                      "Could not get task complete signal within timeout");

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "The ISR did not run");
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed To create Queue");
  TEST_ASSERT_MESSAGE(s_state.enqueue_result, "Failed to enqueue item onto queue");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Failed to dequeue from the task");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xDEADBEEF,
                      "Did not get the expected value after dequeue");
}

static void full_queue_isr_f(void *data) {
  (void)data;
  bool woken = false;
  s_state.enqueue_result = ts_queue_enqueue_from_isr(s_state.queue, (void *)5, &woken);
  s_state.woken = woken;
}

static void full_queue_returns_false_without_blocking(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create Queue");

  // Fill up the queue
  for (uint32_t i = 0; i < QUEUE_SIZE; i++) {
    TEST_ASSERT_MESSAGE(ts_queue_enqueue(s_state.queue, (void *)i, NO_SLEEP),
                        "Failed to enqueue an item on the queue");
  }
  TEST_ASSERT_MESSAGE(ts_queue_get_count(s_state.queue) == QUEUE_SIZE, "Expected a full queue");
  isr_arm(ISR_TRIGGER_PRIORITY, full_queue_isr_f, NULL);
  s_state.enqueue_result = true;
  isr_fire();
  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Exepected the ISR to fire");
  TEST_ASSERT_MESSAGE(!s_state.enqueue_result,
                      "Enqueue operation should have failed on full queue");
  TEST_ASSERT_MESSAGE(!s_state.woken, "No other high priority task should have woken");
}

static void dequeue_one_from_isr_f(void *data) {
  (void)data;
  void *item = 0;
  bool woken = false;
  s_state.dequeue_result = ts_queue_dequeue_from_isr(s_state.queue, &item, &woken);
  s_state.woken = woken;
  s_state.single_dequeued_return_value = (uint32_t)item;
}

static void dequeue_from_isr(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");

  // Enqueue a sentinel element on the queue
  TEST_ASSERT_MESSAGE(ts_queue_enqueue(s_state.queue, (void *)0xDEADBEEF, NO_SLEEP),
                      "Failed to enqueue an item on the queue");
  isr_arm(ISR_TRIGGER_PRIORITY, dequeue_one_from_isr_f, NULL);
  isr_fire();
  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Dequeue operation in the ISR should have succeeded");
  TEST_ASSERT_MESSAGE(!s_state.woken, "No other high priority task should have woken");
}

/**
 * Shared higher-priority consumer. Blocks on s_state.queue and publishes the result.
 *
 * It always signals on completion, but only fixtures that need to *wait* for it should
 * consume that signal. Fixtures asserting that the ISR's yield ran the consumer
 * synchronously must assert straight after isr_fire() -- waiting on the signal there
 * would make them pass even with no yield at all, which is the property under test.
 */
static void consumer_task_f(void *arg) {
  (void)arg;
  void *item = 0;
  s_state.dequeue_result = ts_queue_dequeue(s_state.queue, &item, 100);
  s_state.single_dequeued_return_value = (uint32_t)item;
  signal_send(&s_state.flag);
}

static void unblocking_producer_isr_f(void *data) {
  (void)data;
  bool woken = false;
  s_state.enqueue_result = ts_queue_enqueue_from_isr(s_state.queue, (void *)0xDEADBEEF, &woken);
  s_state.woken = woken;
  task_yield_from_isr(woken);
}

static void higher_priority_consumer_task_awakened_on_enqueue(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");

  // Embunit runner runs at CUTILS_TASK_PRIORITY_MEDIUM
  TEST_TASK_CREATE(consumer_task_f, CUTILS_TASK_PRIORITY_MID_HIGH);
  TEST_ASSERT_MESSAGE(s_state.task, "Failed to create task");
  isr_arm(ISR_TRIGGER_PRIORITY, unblocking_producer_isr_f, NULL);
  isr_fire();
  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(s_state.enqueue_result, "Enqueue in the ISR FAILED");
  TEST_ASSERT_MESSAGE(s_state.woken,
                      "Expected the high priority consumer task to have been woken by the isr");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Dequeue operation in the task Failed");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xDEADBEEF,
                      "Dequeued Value does not match expectation");
}

static void blocking_producer_task_f(void *arg) {
  (void)arg;
  s_state.enqueue_result = ts_queue_enqueue(s_state.queue, (void *)0xC0FEEu, 100);
}

static void unblocking_consumer_isr_f(void *arg) {
  (void)arg;
  bool woken = false;
  void *item = 0;
  s_state.dequeue_result = ts_queue_dequeue_from_isr(s_state.queue, &item, &woken);
  s_state.single_dequeued_return_value = (uint32_t)item;
  s_state.woken = woken;
  task_yield_from_isr(woken);
}

static void higher_priority_producer_task_awakened_on_dequeue(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");

  // Fill up the queue
  for (uint32_t i = 0; i < QUEUE_SIZE; i++) {
    TEST_ASSERT_MESSAGE(ts_queue_enqueue(s_state.queue, (void *)(i + 1), NO_SLEEP),
                        "Failed to enqueue an item on the queue");
  }
  TEST_ASSERT_MESSAGE(ts_queue_get_count(s_state.queue) == QUEUE_SIZE, "Expected a full queue");

  // Start producer thread
  TEST_TASK_CREATE(blocking_producer_task_f, CUTILS_TASK_PRIORITY_MID_HIGH);
  isr_arm(ISR_TRIGGER_PRIORITY, unblocking_consumer_isr_f, NULL);
  isr_fire();

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Dequeue operation in the ISR Failed");
  TEST_ASSERT_MESSAGE(s_state.woken,
                      "Expected the high priority producer task to have been woken by the isr");
  TEST_ASSERT_MESSAGE(s_state.enqueue_result, "Enqueue in the Task FAILED");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 1,
                      "Dequeued Value does not match expectation");
}

typedef struct {
  volatile bool woken_after_first;
  volatile bool woken_after_second;
} accumulator_flags_data_t;

static accumulator_flags_data_t s_extra_data = {0};

static void accumulating_flag_producer_isr_f(void *data) {
  bool woken = false;
  accumulator_flags_data_t *extra_data = (accumulator_flags_data_t *)data;
  ts_queue_enqueue_from_isr(s_state.queue, (void *)0xDEADBEEF, &woken);
  extra_data->woken_after_first = woken;
  ts_queue_enqueue_from_isr(s_state.second_queue, (void *)0xDEADBEEF, &woken);
  extra_data->woken_after_second = woken;
  task_yield_from_isr(woken);
}

static void wake_flag_accumulates_over_enqueue(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");
  TEST_ASSERT_MESSAGE(s_state.second_queue, "Failed to create second queue");
  memset(&s_extra_data, 0, sizeof(s_extra_data));

  TEST_TASK_CREATE(consumer_task_f, CUTILS_TASK_PRIORITY_MID_HIGH);
  isr_arm(ISR_TRIGGER_PRIORITY, accumulating_flag_producer_isr_f, &s_extra_data);
  isr_fire();

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Dequeue operation in the task Failed");
  TEST_ASSERT_MESSAGE(s_extra_data.woken_after_first,
                      "Expected the high priority producer task to have been woken by the isr");
  TEST_ASSERT_MESSAGE(s_extra_data.woken_after_second,
                      "Expected the second enqueue to have not changed the wake request");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xDEADBEEF,
                      "Dequeued Value does not match expectation");
}

static void empty_queue_dequeue_isr_f(void *data) {
  (void)data;
  bool woken = false;
  // Poisoned so the assertions can tell "left alone" from "written with zero".
  void *item = (void *)0xABADCAFE;
  s_state.dequeue_result = ts_queue_dequeue_from_isr(s_state.queue, &item, &woken);
  s_state.single_dequeued_return_value = (uint32_t)item;
  s_state.woken = woken;
  task_yield_from_isr(woken);
}

static void empty_queue_dequeue_returns_false_without_blocking(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");
  TEST_ASSERT_MESSAGE(ts_queue_get_count(s_state.queue) == 0, "Expected an empty queue");

  isr_arm(ISR_TRIGGER_PRIORITY, empty_queue_dequeue_isr_f, NULL);
  // Poisoned so an ISR that never ran cannot satisfy the assertion below.
  s_state.dequeue_result = true;
  isr_fire();

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(!s_state.dequeue_result,
                      "Dequeue should have failed immediately on an empty queue");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xABADCAFE,
                      "A failed dequeue must not write the item out parameter");
  TEST_ASSERT_MESSAGE(!s_state.woken, "No task should have been woken");
}

static void null_flag_isr_f(void *data) {
  (void)data;
  // A NULL flag is documented as "the caller accepts a deferred context switch".
  // The wrapper must not dereference it even though this enqueue really does wake
  // the higher priority consumer. With no flag there is nothing to yield on, so
  // this ISR deliberately does not call task_yield_from_isr().
  s_state.enqueue_result = ts_queue_enqueue_from_isr(s_state.queue, (void *)0xDEADBEEF, NULL);
}

static void null_woken_flag_is_accepted(void) {
  TEST_ASSERT_MESSAGE(s_state.queue, "Failed to create queue");

  TEST_TASK_CREATE(consumer_task_f, CUTILS_TASK_PRIORITY_MID_HIGH);
  TEST_ASSERT_MESSAGE(s_state.task, "Failed to create task");
  isr_arm(ISR_TRIGGER_PRIORITY, null_flag_isr_f, NULL);
  isr_fire();

  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(s_state.enqueue_result, "Enqueue with a NULL woken flag failed");

  // The ISR requested no context switch, so unlike the fixtures above the consumer
  // has not run yet on return from isr_fire(). It runs at the next scheduling
  // point -- blocking the runner here is that point.
  TEST_ASSERT_MESSAGE(signal_wait_timed(&s_state.flag, 100),
                      "Consumer did not run after the deferred context switch");
  TEST_ASSERT_MESSAGE(s_state.dequeue_result, "Consumer failed to dequeue the item");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xDEADBEEF,
                      "Dequeued value does not match expectation");
}

static void null_queue_isr_f(void *data) {
  (void)data;
  bool woken = false;
  void *item = (void *)0xABADCAFE;
  s_state.enqueue_result = ts_queue_enqueue_from_isr(NULL, (void *)0xDEADBEEF, &woken);
  s_state.dequeue_result = ts_queue_dequeue_from_isr(NULL, &item, &woken);
  s_state.single_dequeued_return_value = (uint32_t)item;
  s_state.woken = woken;
  task_yield_from_isr(woken);
}

static void null_queue_fails_without_faulting(void) {
  isr_arm(ISR_TRIGGER_PRIORITY, null_queue_isr_f, NULL);
  // Poisoned so an ISR that never ran cannot satisfy the assertions below.
  s_state.enqueue_result = true;
  s_state.dequeue_result = true;
  isr_fire();

  // Reaching this line is itself half the assertion: dereferencing the NULL queue
  // would hard fault, and HardFault_Handler spins until ctest times out.
  TEST_ASSERT_MESSAGE(isr_did_run_in_handler(), "Expected the ISR to fire");
  TEST_ASSERT_MESSAGE(!s_state.enqueue_result, "Enqueue on a NULL queue should have failed");
  TEST_ASSERT_MESSAGE(!s_state.dequeue_result, "Dequeue on a NULL queue should have failed");
  TEST_ASSERT_MESSAGE(s_state.single_dequeued_return_value == 0xABADCAFE,
                      "A failed dequeue must not write the item out parameter");
  TEST_ASSERT_MESSAGE(!s_state.woken, "No task should have been woken");
}

TestRef queue_ts_queue_isr_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("ISR trigger fires and dispatches", trigger_fires_and_dispatches),
      new_TestFixture("Disarm prevents dispatch", disarm_prevents_dispatch),
      new_TestFixture("Enqueue item from ISR Happy path", happy_path_test),
      new_TestFixture("Enqueue on a full queue in the should fail",
                      full_queue_returns_false_without_blocking),
      new_TestFixture("Dequeue a single item from an isr", dequeue_from_isr),
      new_TestFixture(
          "High priority consumer thread awakened by isr enqueue should indicate in the flag",
          higher_priority_consumer_task_awakened_on_enqueue),
      new_TestFixture(
          "Higher priority producer thread awakened by isr dequeue should indicate in the flag",
          higher_priority_producer_task_awakened_on_dequeue),
      new_TestFixture("Accumulate wake boolen over multiple enqueue isr calls",
                      wake_flag_accumulates_over_enqueue),
      new_TestFixture("Dequeue from an empty queue in an isr should fail",
                      empty_queue_dequeue_returns_false_without_blocking),
      new_TestFixture("A NULL woken flag is accepted and defers the context switch",
                      null_woken_flag_is_accepted),
      new_TestFixture("A NULL queue fails the call without faulting",
                      null_queue_fails_without_faulting)};
  EMB_UNIT_TESTCALLER(
      queue_ts_queue_isr_tests, "queue_ts_queue_isr_test", isr_setUp, isr_tearDown, fixtures);
  return (TestRef)&queue_ts_queue_isr_tests;
}
