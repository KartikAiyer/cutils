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

/*
 * Aggregate test binary that runs ALL suites in one process.
 * Each suite's main() is guarded by #ifndef AGGREGATE_RUNNER so all suite
 * .c files can be compiled together without duplicate symbols.
 */

#include <embUnit/embUnit.h>
#include <stddef.h>

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
/* Note: dispatch_queue, asyncio, and state_event_loop tests
 * are excluded from the aggregate — they rely on task/thread
 * infrastructure not available on the host pthread platform. */

static void test_wrapper(TestRef (*get_tests)(void)) {
  TestRunner_runTest(get_tests());
}

int main(void) {
  TestRunner_start();

  test_wrapper(klist_get_tests);
  test_wrapper(mem_ring_buffer_get_tests);
  test_wrapper(os_mutex_get_tests);
  test_wrapper(os_event_flag_get_tests);
  test_wrapper(os_task_get_tests);
  test_wrapper(queue_free_list_get_tests);
  test_wrapper(queue_kqueue_get_tests);
  test_wrapper(queue_ts_queue_simple_get_tests);
  test_wrapper(queue_ts_queue_get_tests);
  test_wrapper(pool_get_tests);
  test_wrapper(notifier_get_tests);
  test_wrapper(notifier_static_store_get_tests);
  test_wrapper(accumulator_get_tests);

  TestRunner_end();
  return 0;
}
