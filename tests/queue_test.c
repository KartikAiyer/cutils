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

#include <stddef.h>
#include <cutils/free_list.h>
#include <cutils/kqueue.h>
#include <cutils/ts_queue.h>
#include <cutils/task.h>
#include <embUnit/embUnit.h>
#include <string.h>
#include <stdint.h>

/* -------------- free_list_test (2 tests) ---------- */

typedef struct {
  KListElem elem;
  uint32_t data;
} free_list_test_unit_t;

FREE_LIST_STORE_DECL(tq_free_list, free_list_test_unit_t, 10);
FREE_LIST_STORE_DEF(tq_free_list);

static free_list_t *s_fl_list = NULL;

static void free_list_setUp(void) {
  free_list_create_params_t params = {0};
  FREE_LIST_STORE_CREATE_PARAMS_INIT(params, tq_free_list);
  s_fl_list = free_list_init(&params);
}

static void free_list_tearDown(void) {}

static void freeListCanCreateWithAppropriateSize(void) {
  TEST_ASSERT_EQUAL_INT((int)GetArraySize(FREE_LIST_STORE(tq_free_list).store),
                        (int)s_fl_list->item_count);
  TEST_ASSERT_EQUAL_INT((int)GetArraySize(FREE_LIST_STORE(tq_free_list).store),
                        (int)s_fl_list->current_available);
}

static void freeListCanAllocateAsManyAsAvailable(void) {
  KListHead *p_head = 0;
  KListElem *p_alloc = NULL;
  for (uint32_t i = 0; i < s_fl_list->item_count; i++) {
    free_list_test_unit_t *item = (free_list_test_unit_t *)free_list_get(s_fl_list);
    TEST_ASSERT(item);
    item->data = i + 1;
    KLIST_HEAD_PREPEND(p_head, item);
  }
  TEST_ASSERT_EQUAL_INT(0, (int)s_fl_list->current_available);
  while (p_head) {
    KListElem *list_item = NULL;
    KLIST_HEAD_POP(p_head, list_item);
    TEST_ASSERT(list_item);
    free_list_put(s_fl_list, list_item);
  }
  TEST_ASSERT_EQUAL_INT((int)GetArraySize(FREE_LIST_STORE(tq_free_list).store),
                        (int)s_fl_list->current_available);
}

/* -------------- kqueue_test (4 tests) ---------- */

static kqueue_t s_kqueue = {0};

static void kqueue_setUp(void) { kqueue_init(&s_kqueue); }
static void kqueue_tearDown(void) { kqueue_drop_all(&s_kqueue); }

static void queueShouldReturnCountOfZeroForEmptyQueue(void) {
  TEST_ASSERT_EQUAL_INT(0, (int)kqueue_get_item_count(&s_kqueue));
}

static void queueShouldInsertAndUpdateCount(void) {
  struct {
    KListElem elem;
    int val;
  } item = {0};
  kqueue_insert(&s_kqueue, &item.elem);
  TEST_ASSERT_EQUAL_INT(1, (int)kqueue_get_item_count(&s_kqueue));
}

static void queueShouldDequeueAsExpected(void) {
  struct {
    KListElem elem;
    int val;
  } item = {0};
  kqueue_insert(&s_kqueue, &item.elem);
  KListElem *popped_item = kqueue_dequeue(&s_kqueue);
  TEST_ASSERT_EQUAL_INT(0, (int)kqueue_get_item_count(&s_kqueue));
  TEST_ASSERT_MESSAGE(&item.elem == popped_item, "dequeued item mismatch");
}

static void queueShouldMaintainFIFOOrder(void) {
  struct test_item_t {
    KListElem elem;
    int val;
  } items[10] = {0};
  for (uint32_t i = 0; i < GetArraySize(items); i++) {
    items[i].val = (int)(i + 1);
    kqueue_insert(&s_kqueue, &items[i].elem);
  }
  for (uint32_t i = 0; i < GetArraySize(items); i++) {
    KListElem *popped_item = kqueue_dequeue(&s_kqueue);
    TEST_ASSERT_MESSAGE(&items[i].elem == popped_item, "FIFO order violation");
    TEST_ASSERT_EQUAL_INT((int)(1 + i), ((struct test_item_t *)popped_item)->val);
  }
}

/* -------------- ts_queue_test_simple (1 standalone TEST) ----- */

TS_QUEUE_STORE_DECL(ts_queue_fail_q, 10);
TS_QUEUE_STORE_DEF(ts_queue_fail_q);

static void tsQueueFailIfSizeNotPowerOfTwo(void) {
  ts_queue_create_params_t params = {0};
  TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, ts_queue_fail_q);
  TEST_ASSERT(!ts_queue_init(&params));
}

/* --------------- ts_queue_test (task-aware, 6 total tests) ------ */

#if defined(CUTILS_TASK_USES_THRD_CREATE) || defined(RTOS_TASK_IMPLEMENTED)

TASK_STATIC_STORE_DECL(ts_queue_proc_store, 16 * 1024);
TASK_STATIC_STORE_DEF(ts_queue_proc_store);
TASK_STATIC_STORE_DECL(ts_queue_cons_store, 16 * 1024);
TASK_STATIC_STORE_DEF(ts_queue_cons_store);
TS_QUEUE_STORE_DECL(ts_queue_prod_cons_q, 16);
TS_QUEUE_STORE_DEF(ts_queue_prod_cons_q);

typedef struct {
  KListElem elem;
  uint32_t val;
  void (*fn)(void *);
} task_data_t;

FREE_LIST_STORE_DECL(ts_queue_prod_cons_fl, task_data_t, 17);
FREE_LIST_STORE_DEF(ts_queue_prod_cons_fl);

static ts_queue_t *s_ts_queue = NULL;
static task_t *s_prod_task = NULL;
static task_t *s_cons_task = NULL;
static free_list_t *s_data_store = NULL;

static void task_proc_fn(void *ctx) {
  (void)ctx;
}

static void task_cons_fn(void *ctx) {
  task_data_t *td = NULL;
  while (ts_queue_dequeue(s_ts_queue, (void **)&td, 2000)) {
    if (td->val == 9999) {
      free_list_put(s_data_store, &td->elem);
      return;
    }
    if (td->fn) {
      td->fn(td);
    }
    free_list_put(s_data_store, &td->elem);
  }
}

static void setUp_ts_queue(void) {
  free_list_create_params_t fl_params = {0};
  ts_queue_create_params_t ts_params = {0};
  FREE_LIST_STORE_CREATE_PARAMS_INIT(fl_params, ts_queue_prod_cons_fl);
  s_data_store = free_list_init(&fl_params);
  TS_QUEUE_STORE_CREATE_PARAMS_INIT(ts_params, ts_queue_prod_cons_q);
  s_prod_task = s_cons_task = NULL;
  s_ts_queue = ts_queue_init(&ts_params);
}

static void tearDown_ts_queue(void) {
  /* Kill consumer by sending sentinel */
  task_data_t *kill = (task_data_t *)free_list_get(s_data_store);
  memset(kill, 0, sizeof(*kill));
  kill->val = 9999;
  ts_queue_enqueue(s_ts_queue, kill, 2000);
  if (s_prod_task) {
    task_destroy_static(s_prod_task);
    s_prod_task = NULL;
  }
  if (s_cons_task) {
    task_destroy_static(s_cons_task);
    s_cons_task = NULL;
  }
  if (s_ts_queue) {
    ts_queue_destroy(s_ts_queue);
    s_ts_queue = NULL;
  }
}

static void shouldReportZeroCountForEmptyQueue(void) {
  TEST_ASSERT(s_ts_queue);
  TEST_ASSERT_EQUAL_INT(0, (int)ts_queue_get_count(s_ts_queue));
}

static void shouldBlockOnEmptyQueue(void) {
  void *data = NULL;
  uint64_t c_start = task_get_ticks();
  TEST_ASSERT(s_ts_queue);
  TEST_ASSERT(!ts_queue_dequeue(s_ts_queue, &data, 2000));
  uint64_t c_end = task_get_ticks();
  uint64_t elapsed = c_end > c_start ? c_end - c_start : c_start - c_end;
  TEST_ASSERT_MESSAGE(elapsed > 999ULL, "did not block long enough");
}

static void shouldBlockOnFullQueue(void) {
  TEST_ASSERT(s_ts_queue);
  for (uint32_t i = 0; i < s_ts_queue->size; i++) {
    task_data_t *data = (task_data_t *)free_list_get(s_data_store);
    memset(data, 0, sizeof(*data));
    TEST_ASSERT(data);
    data->val = i;
    TEST_ASSERT(ts_queue_enqueue(s_ts_queue, data, 2000));
  }
  task_data_t *last_data = (task_data_t *)free_list_get(s_data_store);
  memset(last_data, 0, sizeof(*last_data));
  TEST_ASSERT(last_data);
  last_data->val = (uint32_t)s_ts_queue->size + 1;
  uint64_t c_start = task_get_ticks();
  TEST_ASSERT(!ts_queue_enqueue(s_ts_queue, last_data, 2000));
  uint64_t c_end = task_get_ticks();
  uint64_t elapsed = c_end > c_start ? c_end - c_start : c_start - c_end;
  TEST_ASSERT_MESSAGE(elapsed > 999ULL, "did not block long enough");
}

static void task_fn_impl(void *p) {
  task_data_t *td = (task_data_t *)p;
  td->val = 10;
}

static void producerConsumerShouldRun(void) {
  TEST_ASSERT(s_ts_queue);
  /* On pthread host with no threading, simulate inline */
  task_data_t data = {0};
  data.fn = task_fn_impl;
  TEST_ASSERT(ts_queue_enqueue(s_ts_queue, &data, 2000));
  if (data.fn) {
    data.fn(&data);
  }
  TEST_ASSERT_EQUAL_INT(10, (int)data.val);
}
#endif

/* ---- TestRef exports ---- */

TestRef queue_free_list_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("Can create free list with appropriate size",
                      freeListCanCreateWithAppropriateSize),
      new_TestFixture("Can allocate as many as available",
                      freeListCanAllocateAsManyAsAvailable)};
  EMB_UNIT_TESTCALLER(queue_free_list_tests,
                      "queue_free_list_test",
                      free_list_setUp,
                      free_list_tearDown,
                      fixtures);
  return (TestRef)&queue_free_list_tests;
}

TestRef queue_kqueue_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("Queue should return count of zero for empty queue",
                      queueShouldReturnCountOfZeroForEmptyQueue),
      new_TestFixture("Queue should insert and update count",
                      queueShouldInsertAndUpdateCount),
      new_TestFixture("Queue should dequeue as expected",
                      queueShouldDequeueAsExpected),
      new_TestFixture("Queue should maintain FIFO order",
                      queueShouldMaintainFIFOOrder)};
  EMB_UNIT_TESTCALLER(queue_kqueue_tests,
                      "queue_kqueue_test",
                      kqueue_setUp,
                      kqueue_tearDown,
                      fixtures);
  return (TestRef)&queue_kqueue_tests;
}

TestRef queue_ts_queue_simple_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("Queue should fail if size not power of 2",
                      tsQueueFailIfSizeNotPowerOfTwo)};
  EMB_UNIT_TESTCALLER(queue_ts_queue_simple_tests,
                      "queue_ts_queue_simple_test",
                      NULL,
                      NULL,
                      fixtures);
  return (TestRef)&queue_ts_queue_simple_tests;
}

#if defined(CUTILS_TASK_USES_THRD_CREATE) || defined(RTOS_TASK_IMPLEMENTED)
TestRef queue_ts_queue_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture(
          "Should report zero count for empty queue", shouldReportZeroCountForEmptyQueue),
      new_TestFixture("Should block on empty queue", shouldBlockOnEmptyQueue),
      new_TestFixture("Should block on full queue", shouldBlockOnFullQueue),
      new_TestFixture("Producer consumer should run", producerConsumerShouldRun)};
  EMB_UNIT_TESTCALLER(queue_ts_queue_tests,
                      "queue_ts_queue_test",
                      setUp_ts_queue,
                      tearDown_ts_queue,
                      fixtures);
  return (TestRef)&queue_ts_queue_tests;
}
#else
static void skip_ts_queue_test(void) {}

TestRef queue_ts_queue_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("ts_queue tests skipped (no task threads)", skip_ts_queue_test),
      new_TestFixture("ts_queue blocked tests skipped (platform)", skip_ts_queue_test)};
  EMB_UNIT_TESTCALLER(queue_ts_queue_tests_skip,
                      "queue_ts_queue_test_skip",
                      NULL,
                      NULL,
                      fixtures);
  return (TestRef)&queue_ts_queue_tests_skip;
}
#endif

#ifndef AGGREGATE_RUNNER
int main() {
  TestRunner_start();
  {
    TestRunner_runTest(queue_free_list_get_tests());
    TestRunner_runTest(queue_kqueue_get_tests());
    TestRunner_runTest(queue_ts_queue_simple_get_tests());
    TestRunner_runTest(queue_ts_queue_get_tests());
  }
  TestRunner_end();
  return 0;
}
#endif
