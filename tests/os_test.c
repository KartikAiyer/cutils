/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2019> <Kartik Aiyer>
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

/*
 * The pthread task platform's task_new_static requires stacks >= the system's
 * default pthread stack size (~8MB on typical Linux). For host CI on the
 * pthread platform, task tests are gated below (matching existing pool_tests
 * behavior). On the C11 platform and RTOS targets, these tests run normally.
 */

#include <cutils/event_flag.h>
#include <cutils/mutex.h>
#include <cutils/task.h>
#include <embUnit/embUnit.h>
#include <string.h>
#include <stdint.h>

/* ---- MutexTest (2 tests) ---- */

static mutex_t s_mtx = {0};

static void mutex_setUp(void) { mutex_new(&s_mtx); }
static void mutex_tearDown(void) { mutex_free(&s_mtx); }

static void mutexApiShouldReturnValidData(void) {
  TEST_ASSERT(mutex_unlock(&s_mtx));
  TEST_ASSERT(mutex_lock(&s_mtx, 0));
  TEST_ASSERT(mutex_unlock(&s_mtx));
}

static void mutexTimedAPIShouldWorkAsExpected(void) {
  TEST_ASSERT(mutex_lock(&s_mtx, 0));
  TEST_ASSERT(!mutex_lock(&s_mtx, 1000));
  TEST_ASSERT(mutex_unlock(&s_mtx));
}

/* ---- EventFlagTest (2 tests) ---- */

static event_flag_t s_evt = {0};

static void eventFlag_setUp(void) { event_flag_new(&s_evt); }
static void eventFlag_tearDown(void) { event_flag_free(&s_evt); }

static void eventFlagAPI(void) {
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x1, WAIT_OR_CLEAR, NULL, NO_SLEEP));
}

static void eventFlagAnswerForCorrectBits(void) {
  uint32_t actual_flags = 0;
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x3, WAIT_OR_CLEAR, &actual_flags, 0));
  TEST_ASSERT_EQUAL_INT(0, (int)actual_flags);
  TEST_ASSERT(event_flag_send(&s_evt, 0x1));
  TEST_ASSERT(event_flag_wait(&s_evt, 0x3, WAIT_OR, &actual_flags, 0));
  TEST_ASSERT_EQUAL_INT(1, (int)actual_flags);
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x3, WAIT_AND, &actual_flags, 0));
  TEST_ASSERT_EQUAL_INT(1, (int)actual_flags);
  TEST_ASSERT(event_flag_wait(&s_evt, 0x3, WAIT_OR_CLEAR, &actual_flags, 0));
  TEST_ASSERT_EQUAL_INT(1, (int)actual_flags);
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x1, WAIT_OR_CLEAR, &actual_flags, 0));
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x3, WAIT_AND, &actual_flags, 0));
  TEST_ASSERT(event_flag_send(&s_evt, 0x3));
  TEST_ASSERT(event_flag_wait(&s_evt, 0x3, WAIT_AND_CLEAR, &actual_flags, 0));
  TEST_ASSERT_EQUAL_INT(0x3, (int)actual_flags);
  TEST_ASSERT(!event_flag_wait(&s_evt, 0x3, WAIT_OR, &actual_flags, 0));
}

/* ---- TaskTest (2 tests) ----
 * Gated: the pthread task platform requires large stacks for task_new_static.
 * These tests run on the C11 and RTOS platforms.
 */

#if defined(CUTILS_TASK_USES_THRD_CREATE) || defined(RTOS_TASK_IMPLEMENTED)

TASK_STATIC_STORE_DECL(test_tsk, 16 * 1024);
TASK_STATIC_STORE_DEF(test_tsk);
TASK_STATIC_STORE_DECL(thread_mid_store, 16 * 1024);
TASK_STATIC_STORE_DEF(thread_mid_store);
TASK_STATIC_STORE_DECL(thread_hi_store, 16 * 1024);
TASK_STATIC_STORE_DEF(thread_hi_store);

static event_flag_t s_task_flag = {0};
static mutex_t s_task_mutex = {0};
static uint32_t s_task_value = 0;

static void test_thread_function(void *arg) {
  (void)arg;
  s_task_value = 1;
}

static void prempt_test_thread_hi(void *arg) {
  (void)arg;
  mutex_lock(&s_task_mutex, WAIT_FOREVER);
  s_task_value = 20;
  mutex_unlock(&s_task_mutex);
  event_flag_send(&s_task_flag, 0x2);
}

static void prempt_test_thread_mid(void *arg) {
  (void)arg;
  uint32_t val = 0;
  bool keepRunning = true;

  while (keepRunning) {
    mutex_lock(&s_task_mutex, WAIT_FOREVER);
    val = s_task_value;
    mutex_unlock(&s_task_mutex);
    task_sleep(1);
    keepRunning = (val == 0);
  }
  event_flag_send(&s_task_flag, 0x1);
}

static void setUp_task_test(void) {
  s_task_value = 0;
  event_flag_new(&s_task_flag);
  mutex_new(&s_task_mutex);
}

static void tearDown_task_test(void) {
  event_flag_free(&s_task_flag);
  mutex_free(&s_task_mutex);
}

static void taskApiTest(void) {
  task_create_params_t params;
  memset(&params, 0, sizeof(params));
  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 test_tsk,
                                 (char *)"Test Thread",
                                 CUTILS_TASK_PRIORITY_MEDIUM,
                                 test_thread_function,
                                 (void *)NULL);
  task_t *p_task = task_new_static(&params);
  TEST_ASSERT(p_task);
  task_start(p_task);
  task_sleep(10);
  TEST_ASSERT_EQUAL_INT(1, (int)s_task_value);
  task_destroy_static(p_task);
}

static void basicPremption(void) {
  task_t *p_task_mid = NULL;
  task_t *p_task_hi = NULL;
  task_create_params_t params;

  memset(&params, 0, sizeof(params));
  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 thread_mid_store,
                                 (char *)"ThreadMid",
                                 CUTILS_TASK_PRIORITY_MEDIUM,
                                 prempt_test_thread_mid,
                                 (void *)NULL);
  p_task_mid = task_new_static(&params);
  TEST_ASSERT(p_task_mid);

  memset(&params, 0, sizeof(params));
  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 thread_hi_store,
                                 (char *)"ThreadHi",
                                 CUTILS_TASK_PRIORITY_MID_HIGH,
                                 prempt_test_thread_hi,
                                 (void *)NULL);
  p_task_hi = task_new_static(&params);
  TEST_ASSERT(p_task_hi);
  task_start(p_task_mid);
  task_start(p_task_hi);
  TEST_ASSERT(event_flag_wait(&s_task_flag, 0x3, WAIT_AND_CLEAR, NULL, 200));
  TEST_ASSERT_EQUAL_INT(20, (int)s_task_value);
  mutex_free(&s_task_mutex);
  event_flag_free(&s_task_flag);
  task_destroy_static(p_task_mid);
  task_destroy_static(p_task_hi);
}
#endif

/* ---- TestRef exports ---- */

TestRef os_mutex_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture(
          "MutexApiShouldReturnValidData", mutexApiShouldReturnValidData),
      new_TestFixture("MutexTimedAPIShouldWorkAsExpected",
                      mutexTimedAPIShouldWorkAsExpected)};
  EMB_UNIT_TESTCALLER(os_mutex_tests, "os_mutex_test", mutex_setUp, mutex_tearDown, fixtures);
  return (TestRef)&os_mutex_tests;
}

TestRef os_event_flag_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("EventFlagAPI", eventFlagAPI),
      new_TestFixture(
          "EventFlagAnswerForCorrectBits", eventFlagAnswerForCorrectBits)};
  EMB_UNIT_TESTCALLER(os_event_flag_tests,
                      "os_event_flag_test",
                      eventFlag_setUp,
                      eventFlag_tearDown,
                      fixtures);
  return (TestRef)&os_event_flag_tests;
}

#if defined(CUTILS_TASK_USES_THRD_CREATE) || defined(RTOS_TASK_IMPLEMENTED)
TestRef os_task_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("TaskApiTest", taskApiTest),
      new_TestFixture("BasicPremption", basicPremption)};
  EMB_UNIT_TESTCALLER(os_task_tests,
                      "os_task_test",
                      setUp_task_test,
                      tearDown_task_test,
                      fixtures);
  return (TestRef)&os_task_tests;
}
#else
/* Provide a no-op test caller when task tests are disabled */
static void skip_task_test(void) {}
static void nop_setup(void) {}
static void nop_teardown(void) {}

TestRef os_task_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("TaskApiTest (skipped on this platform)", skip_task_test),
      new_TestFixture("BasicPremption (skipped on this platform)", skip_task_test)};
  EMB_UNIT_TESTCALLER(os_task_tests,
                      "os_task_test",
                      nop_setup,
                      nop_teardown,
                      fixtures);
  return (TestRef)&os_task_tests;
}
#endif

#ifndef AGGREGATE_RUNNER
int main() {
  TestRunner_start();
  {
    TestRunner_runTest(os_mutex_get_tests());
    TestRunner_runTest(os_event_flag_get_tests());
    TestRunner_runTest(os_task_get_tests());
  }
  TestRunner_end();
  return 0;
}
#endif
