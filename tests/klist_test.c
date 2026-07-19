/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2019> <Kartik Aiyer>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit the Software is
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

#include <cutils/klist.h>
#include <embUnit/embUnit.h>
#include <string.h>

struct test_obj {
  KListElem elem;
  uint32_t val;
};

static void count_up_f(KListElem *elem, ...) {
  (void)elem;
  va_list args;
  va_start(args, elem);
  uint32_t *p_count = va_arg(args, uint32_t *);
  (*p_count)++;
  va_end(args);
}

static void klist_must_insert_test(void) {
  struct test_obj bufs[10];
  memset(bufs, 0, sizeof(bufs));
  KListHead *p_head = NULL;
  for (uint32_t i = 0; i < sizeof(bufs) / sizeof(bufs[0]); i++) {
    bufs[i].val = 10 + i;
    KLIST_HEAD_PREPEND(p_head, &bufs[i].elem);
  }

  uint32_t num_of_elements = 0;
  KLIST_HEAD_FOREACH(p_head, count_up_f, &num_of_elements);
  TEST_ASSERT_EQUAL_INT((uint32_t)(sizeof(bufs) / sizeof(bufs[0])), num_of_elements);

  uint32_t i = 1;
  while (p_head) {
    struct test_obj *elem = NULL;
    KLIST_HEAD_POP(p_head, elem);
    TEST_ASSERT_EQUAL_INT(10 + (num_of_elements - i), elem->val);
    i++;
  }
}

static void setUp(void) {}
static void tearDown(void) {}

TestRef klist_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture("MustInsert", klist_must_insert_test)};
  EMB_UNIT_TESTCALLER(klist_tests, "klist_test", setUp, tearDown, fixtures);
  return (TestRef)&klist_tests;
}

#ifndef AGGREGATE_RUNNER
int main() {
  TestRunner_start();
  {
    TestRunner_runTest(klist_get_tests());
  }
  TestRunner_end();
  return 0;
}
#endif
