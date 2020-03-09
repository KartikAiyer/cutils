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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DACUTILSS OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <embUnit/embUnit.h>
#include <cutils/accumulator.h>

ACCUMULATOR_STORE_DECL(test, 6);
ACCUMULATOR_STORE_DEF(test);
static accumulator_h s_test_handle;

static void setup(void)
{
  accumulator_create_params_t params = {0};
  ACCUMULATOR_CREATE_PARAMS_INIT(params, test);

  s_test_handle = accumulator_create(&params);
}

static void teardown(void)
{}

static void check_acc_api(void)
{
  accumulator_h handle = s_test_handle;
  TEST_ASSERT(handle);
  uint8_t test_bufA[] = {1, 2};
  TEST_ASSERT(accumulator_insert(handle, test_bufA, sizeof(test_bufA)));
  TEST_ASSERT(accumulator_bytes_contained(handle) == 2);
  TEST_ASSERT(accumulator_bytes_left(handle) == 3);
  uint8_t read_buf[5] = {0};
  TEST_ASSERT(!accumulator_peek(handle, read_buf, sizeof(read_buf)));
  TEST_ASSERT(accumulator_peek(handle, read_buf, 1));
  TEST_ASSERT(read_buf[0] == test_bufA[0]);
  memset(read_buf, 0, sizeof(read_buf));
  TEST_ASSERT(accumulator_peek(handle, read_buf, sizeof(test_bufA)));
  TEST_ASSERT(!memcmp(read_buf, test_bufA, sizeof(test_bufA)));

  memset(read_buf, 0, sizeof(read_buf));
  TEST_ASSERT(accumulator_extract(handle, read_buf, sizeof(test_bufA)));
  TEST_ASSERT(!memcmp(read_buf, test_bufA, sizeof(test_bufA)));
  TEST_ASSERT(accumulator_bytes_left(handle) == 5);
  TEST_ASSERT(accumulator_bytes_contained(handle) == 0);
}

static void check_wrap_around(void)
{
  accumulator_h handle = s_test_handle;

  TEST_ASSERT(handle);
  uint8_t test_bufA[] = {1, 2, 3, 4, 5};
  TEST_ASSERT(accumulator_insert(handle, test_bufA, sizeof(test_bufA)));

  TEST_ASSERT(accumulator_bytes_contained(handle) == sizeof(test_bufA));
  TEST_ASSERT(accumulator_bytes_left(handle) == 0);
  uint8_t read_buf[sizeof(test_bufA)] = {0};
  TEST_ASSERT(accumulator_peek(handle, read_buf, sizeof(read_buf)));
  TEST_ASSERT(!memcmp(read_buf, test_bufA, sizeof(test_bufA)));
  memset(read_buf, 0, sizeof(read_buf));

  //Add a 6
  uint8_t add_one = 6;
  TEST_ASSERT(accumulator_insert(handle, &add_one, sizeof(add_one)));
  TEST_ASSERT(accumulator_peek(handle, read_buf, sizeof(read_buf)));
  uint8_t cmp_buf[] = {2, 3, 4, 5, 6};
  TEST_ASSERT(!memcmp(read_buf, cmp_buf, sizeof(cmp_buf)));

  uint8_t add_two[] = {7, 8};
  TEST_ASSERT(accumulator_insert(handle, add_two, sizeof(add_two)));
  TEST_ASSERT(accumulator_peek(handle, read_buf, sizeof(read_buf)));
  TEST_ASSERT(read_buf[0] == 4 && read_buf[1] == 5 && read_buf[2] == 6 && read_buf[3] == 7 && read_buf[4] == 8);
}

static void check_iterator(void)
{
  accumulator_h handle = s_test_handle;

  TEST_ASSERT(handle);
  uint8_t test_bufA[] = {1, 2, 3, 4, 5};
  TEST_ASSERT(accumulator_insert(handle, test_bufA, sizeof(test_bufA)));
  TEST_ASSERT(accumulator_bytes_contained(handle) == sizeof(test_bufA));

  accumulator_iterator_t iter = accumulator_iterator_init(handle);
  uint8_t read_val = 0;
  for (uint32_t i = 0; i < sizeof(test_bufA); i++) {
    accumulator_peek_at(handle, iter, &read_val, 1);
    TEST_ASSERT(read_val == test_bufA[i]);
    TEST_ASSERT(accumulator_iterator_next(handle, &iter));
  }
  TEST_ASSERT(!accumulator_iterator_next(handle, &iter));
}

static void check_iterator_advances_to_the_end(void)
{
  accumulator_h handle = s_test_handle;
  uint8_t test_bufA[] = {1, 2, 3, 4, 5};
  TEST_ASSERT(accumulator_insert(handle, test_bufA, sizeof(test_bufA)));
  TEST_ASSERT(accumulator_bytes_contained(handle) == sizeof(test_bufA));

  accumulator_iterator_t iter = accumulator_iterator_init(handle);
  TEST_ASSERT(accumulator_iterator_advance(handle, &iter, sizeof(test_bufA) - 1));
  uint8_t val;
  TEST_ASSERT(accumulator_peek_at(handle, iter, &val, 1));
  TEST_ASSERT(val == 5);
}

TestRef accumulator_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixture) {
      new_TestFixture("Accumulator API Tests 1", check_acc_api),
      new_TestFixture("Accumulator Wraps around", check_wrap_around),
      new_TestFixture("Accumulator Iterator Works", check_iterator),
      new_TestFixture("Iterator advances to end", check_iterator_advances_to_the_end)
  };
  EMB_UNIT_TESTCALLER(accumulator_tests, "Accumulator tests", setup, teardown, fixture);
  return (TestRef) &accumulator_tests;
}

int main()
{
  TestRunner_start();
  {
    TestRunner_runTest(accumulator_get_tests());
  }
  TestRunner_end();
  return 0;
}
