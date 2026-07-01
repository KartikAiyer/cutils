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

#include <cutils/ring_buffer.h>
#include <embUnit/embUnit.h>
#include <string.h>

/* MEM_RING_BUFFER_STORE_DECL has a struct ring_buffer field that needs C compatibility */
#ifndef _RING_BUFFER_FWD
typedef struct ring_buffer ring_buffer;
#define _RING_BUFFER_FWD
#endif

#define TEST_BUFFER_SIZE (50)
MEM_RING_BUFFER_STORE_DECL(my_test_buffer, TEST_BUFFER_SIZE);
MEM_RING_BUFFER_STORE_DEF(my_test_buffer);

static ring_buffer_ref s_ring_buffer = NULL;

static void setUp(void) {
  ring_buffer_create_params_t params;
  MEM_RING_BUFFER_CREATE_PARAMS_INIT(params, my_test_buffer);
  s_ring_buffer = create_ring_buffer(&params);
}

static void tearDown(void) { ring_buffer_release(s_ring_buffer); }

static void should_be_created_with_proper_size_test(void) {
  TEST_ASSERT_EQUAL_INT(TEST_BUFFER_SIZE, ring_buffer_get_size(s_ring_buffer));
}

static void stores_elements_in_order_of_insertion_test(void) {
  uint8_t buf[20] = {0};
  uint8_t buf2[20] = {0};
  for (uint32_t i = 0; i < GetArraySize(buf); i++) {
    buf[i] = i + 1;
  }
  TEST_ASSERT_EQUAL_INT(sizeof(buf),
                        ring_buffer_write_data(s_ring_buffer, buf, sizeof(buf)));
  uint32_t bytes_to_read = ring_buffer_get_bytes_available_for_read(s_ring_buffer);
  TEST_ASSERT_EQUAL_INT(sizeof(buf), (uint32_t)bytes_to_read);
  ring_buffer_data_t range = ring_buffer_get_data(s_ring_buffer, bytes_to_read, false);
  TEST_ASSERT_EQUAL_INT(1, (int)range.entryCount);
  TEST_ASSERT_EQUAL_INT(bytes_to_read, range.total_bytes);
  copy_ring_data_to_buffer(&range, buf2);
  TEST_ASSERT_EQUAL_INT(0, memcmp(buf, buf2, sizeof(buf)));
}

static void should_wrap_as_expected_test(void) {
  uint8_t buf[TEST_BUFFER_SIZE] = {0};
  uint8_t buf2[TEST_BUFFER_SIZE] = {0};
  uint8_t buf3[TEST_BUFFER_SIZE] = {0};
  for (uint32_t i = 0; i < TEST_BUFFER_SIZE; i++) {
    buf[i] = i + 1;
    buf2[i] = TEST_BUFFER_SIZE + i + 1;
  }
  TEST_ASSERT_EQUAL_INT(sizeof(buf),
                        ring_buffer_write_data(s_ring_buffer, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_INT(0, ring_buffer_get_bytes_available_for_write(s_ring_buffer));
  uint32_t bytes_to_overwrite = 10;
  ring_buffer_clear_data(s_ring_buffer, bytes_to_overwrite);
  TEST_ASSERT_EQUAL_INT(
      bytes_to_overwrite,
      ring_buffer_write_data(s_ring_buffer, buf2, bytes_to_overwrite));
  uint32_t bytes_to_read = ring_buffer_get_bytes_available_for_read(s_ring_buffer);
  TEST_ASSERT_EQUAL_INT(TEST_BUFFER_SIZE, (uint32_t)bytes_to_read);

  ring_buffer_data_t range = ring_buffer_get_data(s_ring_buffer, bytes_to_read, false);
  TEST_ASSERT_EQUAL_INT(2, (int)range.entryCount);
  TEST_ASSERT_EQUAL_INT(bytes_to_read, range.total_bytes);
  copy_ring_data_to_buffer(&range, buf3);
  TEST_ASSERT_EQUAL_INT(0,
                        memcmp(&buf[bytes_to_overwrite],
                               buf3,
                               sizeof(buf) - bytes_to_overwrite));
  TEST_ASSERT_EQUAL_INT(
      0, memcmp(buf2, &buf3[TEST_BUFFER_SIZE - bytes_to_overwrite], bytes_to_overwrite));
}

TestRef mem_ring_buffer_get_tests(void) {
  EMB_UNIT_TESTFIXTURES(fixtures){
      new_TestFixture(
          "Should be created with proper size", should_be_created_with_proper_size_test),
      new_TestFixture(
          "Stores elements in order of insertion", stores_elements_in_order_of_insertion_test),
      new_TestFixture("Should wrap as expected", should_wrap_as_expected_test)};
  EMB_UNIT_TESTCALLER(mem_ring_buffer_test,
                      "mem_ring_buffer_test",
                      setUp,
                      tearDown,
                      fixtures);
  return (TestRef)&mem_ring_buffer_test;
}

#ifndef AGGREGATE_RUNNER
int main() {
  TestRunner_start();
  {
    TestRunner_runTest(mem_ring_buffer_get_tests());
  }
  TestRunner_end();
  return 0;
}
#endif
