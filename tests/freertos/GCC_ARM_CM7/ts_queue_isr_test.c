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
#include <string.h>

static struct {
  volatile bool ran;
  void *volatile seen_data;
} s_smoke;

static uint32_t s_sentinel = 0xC0FFEEu;

static void smoke_isr(void *data) {
  s_smoke.ran = true;
  s_smoke.seen_data = data;
}

static void setUp(void) { memset(&s_smoke, 0, sizeof(s_smoke)); }
static void tearDown(void) { isr_disarm(); }

static void triggerFiresAndDispatches(void) {
  isr_arm(ISR_TRIGGER_PRIORITY, smoke_isr, &s_sentinel);
  TEST_ASSERT(!isr_did_run_in_handler());
  isr_fire();
  TEST_ASSERT(isr_did_run_in_handler());
  TEST_ASSERT(s_smoke.ran);
  TEST_ASSERT(s_smoke.seen_data == &s_sentinel);
}

static void disarmPreventsDispatch(void) {
  isr_arm(ISR_TRIGGER_PRIORITY, smoke_isr, &s_sentinel);
  isr_fire();
  TEST_ASSERT(s_smoke.ran);

  s_smoke.ran = false;
  isr_disarm();
  isr_fire();
  TEST_ASSERT(!s_smoke.ran);
}

TestRef queue_ts_queue_isr_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("ISR trigger fires and dispatches", triggerFiresAndDispatches),
      new_TestFixture("Disarm prevents dispatch", disarmPreventsDispatch)};
  EMB_UNIT_TESTCALLER(
      queue_ts_queue_isr_tests, "queue_ts_queue_isr_test", setUp, tearDown, fixtures);
  return (TestRef)&queue_ts_queue_isr_tests;
}
