/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2020> <Kartik Aiyer>
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

#include <cutils/dispatch_queue.h>
#include <cutils/event_flag.h>
#include <embUnit/embUnit.h>
#include <stdio.h>

DISPATCH_QUEUE_STORE_DECL(test_queue_1, 16, 4096);
DISPATCH_QUEUE_STORE_DEF(test_queue_1);

DISPATCH_QUEUE_STORE_DECL(test_queue_2, 4, 4096);
DISPATCH_QUEUE_STORE_DEF(test_queue_2);

DISPATCH_QUEUE_STORE_DECL(test_queue_3, 32, 4096);
DISPATCH_QUEUE_STORE_DEF(test_queue_3);

typedef struct
{
  uint32_t sleep_time;
  uint32_t val;
} action_data_t;

static void action_1(void *arg1, void *arg2)
{
  action_data_t *p_data = (action_data_t *) arg1;
  task_sleep(p_data->sleep_time);
  p_data->val = 1;
}

static void test_queue(dispatch_queue_t *p_queue)
{
  action_data_t data = {.sleep_time = 5};
  TEST_ASSERT(dispatch_async_f(p_queue, action_1, &data, NULL));
  dispatch_queue_destroy(p_queue);
  TEST_ASSERT_MESSAGE(data.val == 1, "Action should modify test value on dispatch queue");
}

static void can_create_and_run_action(void)
{
  dispatch_queue_create_params_t params = {0};
  dispatch_queue_t *p_queue = 0;
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_1, "test_dispatch_queue1", CUTILS_TASK_PRIORITY_MEDIUM);
  p_queue = dispatch_queue_create(&params);
  TEST_ASSERT_MESSAGE(p_queue, "Dispatch queue should be created successfully");
  test_queue(p_queue);
  memset(&params, 0, sizeof(params));

  //Task 2
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_2, "test_dispatch_queue2", CUTILS_TASK_PRIORITY_MID_HIGH);
  p_queue = dispatch_queue_create(&params);
  TEST_ASSERT_MESSAGE(p_queue, "Dispatch queue should be created successfully");
  test_queue(p_queue);
  memset(&params, 0, sizeof(params));

  //Task 3
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_3, "test_dispatch_queue3", CUTILS_TASK_PRIORITY_MID_HIGH);
  p_queue = dispatch_queue_create(&params);
  TEST_ASSERT_MESSAGE(p_queue, "Dispatch queue should be created successfully");
  test_queue(p_queue);
  memset(&params, 0, sizeof(params));
}

typedef struct
{
  dispatch_queue_t *p_queue_hi, *p_queue_lo;
  uint32_t val;
} task_action_test_data_t;

static void lo_action(void *arg1, void *arg2)
{
  task_action_test_data_t *p_data = (task_action_test_data_t *) arg1;
  p_data->val--;
}

static void hi_action(void *arg1, void *arg2)
{
  task_action_test_data_t *p_data = (task_action_test_data_t *) arg1;

  TEST_ASSERT(p_data);
  p_data->val++;
  p_data->val++;
}

static void medium_action(void *arg1, void *arg2)
{
  task_action_test_data_t *p_data = (task_action_test_data_t *) arg1;

  TEST_ASSERT(p_data);
  for (uint32_t i = 0; i < 4; i++) {
    TEST_ASSERT(dispatch_async_f(p_data->p_queue_hi, hi_action, p_data, NULL));
  }
  for (uint32_t i = 0; i < 4; i++) {
    TEST_ASSERT(dispatch_async_f(p_data->p_queue_lo, lo_action, p_data, NULL));
  }
  task_sleep(100);
}

static void can_post_between_queues(void)
{
  task_action_test_data_t data = {0};
  dispatch_queue_t *p_queue_mid = 0;
  dispatch_queue_create_params_t params = {0};

  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_1, "test exec hi", CUTILS_TASK_PRIORITY_HIGHEST);
  data.p_queue_hi = dispatch_queue_create(&params);
  TEST_ASSERT(data.p_queue_hi);

  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_2, "test exec mid", CUTILS_TASK_PRIORITY_MEDIUM);
  p_queue_mid = dispatch_queue_create(&params);
  TEST_ASSERT(p_queue_mid);

  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_3, "test exec lo", CUTILS_TASK_PRIORITY_MID_LO);
  data.p_queue_lo = dispatch_queue_create(&params);
  TEST_ASSERT(data.p_queue_lo);

  TEST_ASSERT(dispatch_async_f(p_queue_mid, medium_action, &data, NULL));

  dispatch_queue_destroy(p_queue_mid);
  dispatch_queue_destroy(data.p_queue_lo);
  dispatch_queue_destroy(data.p_queue_hi);
  TEST_ASSERT_EQUAL_INT(4, data.val);
}
#if 0
typedef struct _dispatch_queue_isr_test_t
{
  AMBA_TIMER_ID_e timer;
  atomic_uint val;
  dispatch_queue_t *p_queue;
  event_flag_t flag;
}dispatch_queue_isr_test_t;

static dispatch_queue_isr_test_t s_isr_test = { 0 };

#define      DQI_TEST_FLAG_ISR_SUCCESS   (1 << 0)
#define      DQI_TEST_FLAG_ACTION_SUCCESS  (1 << 1)
#define      DQI_TEST_FLAG_ISR_ERROR     (1 << 2)

static void dispatch_isr_test_action(void* arg1, void* arg2)
{
  dispatch_queue_isr_test_t* p_test_data = (dispatch_queue_isr_test_t*)arg1;
  atomic_fetch_add_explicit(&p_test_data->val, 1, memory_order_relaxed);
  event_flag_send(&p_test_data->flag, DQI_TEST_FLAG_ACTION_SUCCESS);
}
static void timer_isr(AMBA_TIMER_ID_e timer, UINT32 private)
{
  dispatch_queue_isr_test_t* p_test_data = (dispatch_queue_isr_test_t*)private;
  if( dispatch_async_f(p_test_data->p_queue, dispatch_isr_test_action, p_test_data, NULL) ) {
    event_flag_send(&p_test_data->flag, DQI_TEST_FLAG_ISR_SUCCESS);
  } else {
    event_flag_send(&p_test_data->flag, DQI_TEST_FLAG_ISR_ERROR);
  }
}

static void can_post_from_isr(void)
{
  AMBA_TIMER_ID_e timer_id = AMBA_TIMER3;
  AMBA_TIMER_CONFIG_s timer_config = { .CounterVal =  500,
                                       .IntervalVal = 0,
                                       .ExpirationFunc = timer_isr,
                                       .ExpirationArg = (UINT32)&s_isr_test};
  dispatch_queue_create_params_t queue_params = { 0 };
  uint32_t event_flag_params = 0;
  memset(&s_isr_test, 0, sizeof(s_isr_test));
  event_flag_new(&s_isr_test.flag);
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(queue_params, test_queue_1, "isr_test_queue", CUTILS_TASK_PRIORITY_MEDIUM);
  s_isr_test.p_queue = dispatch_queue_create(&queue_params);
  TEST_ASSERT_MESSAGE(s_isr_test.p_queue, "Failed to create dispatch queue");
  TEST_ASSERT_MESSAGE(AmbaRTSL_TimerSet(timer_id, &timer_config) == OK, "Failed to create high res timer");
  TEST_ASSERT_MESSAGE(event_flag_wait(&s_isr_test.flag,
                                      DQI_TEST_FLAG_ISR_ERROR | DQI_TEST_FLAG_ISR_SUCCESS,
                                      WAIT_OR_CLEAR,
                                      &event_flag_params,
                                      100),
                      "Failed to wait on isr interrupt");
  TEST_ASSERT_MESSAGE(!(event_flag_params & DQI_TEST_FLAG_ISR_ERROR), "Failed to post to dispatch queue in isr");
  TEST_ASSERT_MESSAGE(event_flag_wait(&s_isr_test.flag, DQI_TEST_FLAG_ACTION_SUCCESS, WAIT_OR_CLEAR, NULL, 100), "Failed to wait on dispatch complete");
  dispatch_queue_destroy(s_isr_test.p_queue);
  event_flag_free(&s_isr_test.flag);
  TEST_ASSERT(atomic_load_explicit(&s_isr_test.val, memory_order_acquire) == 1);
}

typedef struct _dispatch_queue_app_timer_test_t
{
  AMBA_KAL_TIMER_t app_timer;
  atomic_uint val;
  dispatch_queue_t *p_queue;
  event_flag_t flag;
} dispatch_queue_app_timer_test_t;

#define      DQAT_TEST_FLAG_TIMER_SUCCESS   (1 << 0)
#define      DQAT_TEST_FLAG_ACTION_SUCCESS  (1 << 1)
#define      DQAT_TEST_FLAG_TIMER_ERROR     (1 << 2)

static void dispatch_app_timer_test_action(void *arg1, void *arg2)
{
  dispatch_queue_app_timer_test_t *p_test_data = (dispatch_queue_app_timer_test_t *) arg1;
  atomic_fetch_add_explicit(&p_test_data->val, 1, memory_order_relaxed);
  event_flag_send(&p_test_data->flag, DQAT_TEST_FLAG_ACTION_SUCCESS);
  AmbaKAL_TimerDelete(&p_test_data->app_timer);
}

static void application_timer_f(UINT32 ctx)
{
  dispatch_queue_app_timer_test_t *p_test_data = (dispatch_queue_app_timer_test_t *) ctx;
  if(dispatch_async_f(p_test_data->p_queue, dispatch_app_timer_test_action, p_test_data, NULL)) {
    event_flag_send(&p_test_data->flag, DQAT_TEST_FLAG_TIMER_SUCCESS);
  } else {
    event_flag_send(&p_test_data->flag, DQAT_TEST_FLAG_TIMER_ERROR);
  }
}

static dispatch_queue_app_timer_test_t s_timer_test = {0};

static void can_post_from_application_timer_callback(void)
{
  dispatch_queue_create_params_t queue_params = {0};
  uint32_t ev_params = 0;
  memset(&s_timer_test, 0, sizeof(s_timer_test));

  TEST_ASSERT(event_flag_new(&s_timer_test.flag));
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(queue_params, test_queue_1, "timer_test_queue", CUTILS_TASK_PRIORITY_MEDIUM);
  s_timer_test.p_queue = dispatch_queue_create(&queue_params);
  TEST_ASSERT_MESSAGE(s_timer_test.p_queue, "Failed to create dispatch queue");
  TEST_ASSERT_MESSAGE(AmbaKAL_TimerCreate(&s_timer_test.app_timer,
                                          1,
                                          application_timer_f,
                                          (UINT32) &s_timer_test,
                                          20, 0) == OK,
                      "Failed to create Timer");
  TEST_ASSERT(event_flag_wait(&s_timer_test.flag,
                              DQAT_TEST_FLAG_TIMER_ERROR | DQAT_TEST_FLAG_TIMER_SUCCESS,
                              WAIT_OR_CLEAR,
                              &ev_params,
                              100));
  TEST_ASSERT_MESSAGE(!(ev_params & 0x4), "Failt to post to dispatch from application timer callback");
  TEST_ASSERT(event_flag_wait(&s_timer_test.flag, DQAT_TEST_FLAG_ACTION_SUCCESS, WAIT_OR_CLEAR, NULL, 100));
  dispatch_queue_destroy(s_timer_test.p_queue);
  event_flag_free(&s_timer_test.flag);
  TEST_ASSERT(atomic_load_explicit(&s_timer_test.val, memory_order_acquire) == 1);
}
#endif
static void setup(void)
{
}

static void teardown(void)
{
}

TestRef dispatch_queue_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixture) {
      new_TestFixture("Can Create and run an action on three different dispatch queues", can_create_and_run_action),
      new_TestFixture("Can post between queues", can_post_between_queues),
#if 0
      new_TestFixture("Can post from isr", can_post_from_isr),
      new_TestFixture("Can post from Application Timer", can_post_from_application_timer_callback)
#endif
  };
  EMB_UNIT_TESTCALLER(dispatch_queue_tests, "dispatch_queue_tests", setup, teardown, fixture);
  return (TestRef) &dispatch_queue_tests;
}
#if 0
typedef struct _dispatch_after_test_data_t
{
  atomic_uint count;
  dispatch_queue_t *p_queue;
  signal_t signal;
} dispatch_after_test_data_t;

static dispatch_after_test_data_t s_after_test;

static void timed_action_f(void *arg1, void *arg2)
{
  dispatch_after_test_data_t *p_data = (dispatch_after_test_data_t *) arg1;
  atomic_fetch_add_explicit(&p_data->count, 1, memory_order_relaxed);
  signal_send(&s_after_test.signal);
}

static void can_post_after_timeout(void)
{
  TEST_ASSERT(s_after_test.p_queue);

  dispatch_after_f(s_after_test.p_queue, 10, timed_action_f, &s_after_test, NULL);
  TEST_ASSERT(signal_wait_timed(&s_after_test.signal, 15));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&s_after_test.count));
}

static void can_post_repeatedly(void)
{
  TEST_ASSERT(s_after_test.p_queue);
  const uint32_t NUM_REPETITIONS = 3;
  const uint32_t TIMEOUT = 2;
  dispatch_queue_timed_action_h h_action = dispatch_start_repeated_f(s_after_test.p_queue,
                                                                     TIMEOUT,
                                                                     TIMEOUT,
                                                                     timed_action_f,
                                                                     &s_after_test,
                                                                     NULL);
  TEST_ASSERT(h_action);
  for( uint32_t i = 0; i < NUM_REPETITIONS; i++) {
    TEST_ASSERT(signal_wait_timed(&s_after_test.signal, TIMEOUT + 5));
  }
  dispatch_stop_repeated_f(h_action);
  TEST_ASSERT(!signal_wait_timed(&s_after_test.signal, TIMEOUT + 5));
  TEST_ASSERT_EQUAL_INT(NUM_REPETITIONS, atomic_load(&s_after_test.count));
}

static void setup_timer(void)
{
  memset(&s_after_test, 0, sizeof(s_after_test));
  if (signal_new(&s_after_test.signal)) {
    dispatch_queue_create_params_t params;

    DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, test_queue_1, "dispatch_after_test_queue", CUTILS_TASK_PRIORITY_MEDIUM);
    s_after_test.p_queue = dispatch_queue_create(&params);
  }
}

static void teardown_timer(void)
{
  dispatch_queue_destroy(s_after_test.p_queue);
  signal_free(&s_after_test.signal);
}

TestRef dispatch_queue_timer_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixture) {
      new_TestFixture("Test dispatch_after_f", can_post_after_timeout),
      new_TestFixture("Test dispatch_start_repeated_f", can_post_repeatedly)
  };
  EMB_UNIT_TESTCALLER(queue_timer_tests, "dispatch_queue timer tests", setup_timer, teardown_timer, fixture);
  return (TestRef) &queue_timer_tests;
}
#endif

int main() {
  TestRunner_start();
  {
    TestRunner_runTest(dispatch_queue_get_tests());
  }
  TestRunner_end();
  return 0;
}
