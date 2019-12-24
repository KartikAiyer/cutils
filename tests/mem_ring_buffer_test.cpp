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
#include <gtest/gtest.h>

#define TEST_BUFFER_SIZE              (50)
MEM_RING_BUFFER_STORE_DECL(test_buffer, TEST_BUFFER_SIZE);
MEM_RING_BUFFER_STORE_DEF(test_buffer);
namespace tests {
class MemRingBufferObj : public ::testing::Test {
protected:
  void SetUp() override
  {
    ring_buffer_create_params_t params;
    MEM_RING_BUFFER_CREATE_PARAMS_INIT(params, test_buffer);
    this->ring_buffer = create_ring_buffer(&params);
  }

  void TearDown() override
  {
    ring_buffer_release(this->ring_buffer);
  }
protected:
  ring_buffer_ref ring_buffer;
};

TEST_F(MemRingBufferObj, ShouldBeCreatedWithProperSize)
{
  ASSERT_EQ(TEST_BUFFER_SIZE, ring_buffer_get_size(this->ring_buffer));
}

TEST_F(MemRingBufferObj, StoresElementsInOrderOfInsertion)
{
  uint8_t buf[20] = {0};
  uint8_t buf2[20] = {0};
  for(uint32_t i = 0; i < GetArraySize(buf); i++) {
    buf[i] = i+1;
  }
  ASSERT_EQ(sizeof(buf), ring_buffer_write_data(this->ring_buffer, buf, sizeof(buf)));
  auto bytes_to_read = ring_buffer_get_bytes_available_for_read(this->ring_buffer);
  ASSERT_EQ(sizeof(buf), bytes_to_read);
  auto range = ring_buffer_get_data(this->ring_buffer, bytes_to_read, false);
  ASSERT_EQ(1, range.entryCount); // There should only be one section in the ring buffer
  ASSERT_EQ(bytes_to_read, range.total_bytes);
  copy_ring_data_to_buffer(&range, buf2);
  ASSERT_EQ(0, memcmp(buf, buf2, sizeof(buf)));

}
TEST_F(MemRingBufferObj, ShouldWrapAsExpected)
{
  uint8_t buf[TEST_BUFFER_SIZE] = {0};
  uint8_t buf2[TEST_BUFFER_SIZE] = {0};
  uint8_t buf3[TEST_BUFFER_SIZE] = {0};
  for(uint32_t i = 0 ; i < TEST_BUFFER_SIZE; i++) {
    buf[i] = i+1;
    buf2[i] = TEST_BUFFER_SIZE + i + 1;
  }
  ASSERT_EQ(sizeof(buf), ring_buffer_write_data(this->ring_buffer, buf, sizeof(buf)));
  ASSERT_EQ(0, ring_buffer_get_bytes_available_for_write(this->ring_buffer));
  uint32_t bytes_to_overwrite = 10;
  ring_buffer_clear_data(this->ring_buffer, bytes_to_overwrite);
  ASSERT_EQ(bytes_to_overwrite, ring_buffer_write_data(this->ring_buffer, buf2, bytes_to_overwrite));
  auto bytes_to_read = ring_buffer_get_bytes_available_for_read(this->ring_buffer);
  ASSERT_EQ(TEST_BUFFER_SIZE, bytes_to_read);

  auto range = ring_buffer_get_data(this->ring_buffer, bytes_to_read, false);
  ASSERT_EQ(2, range.entryCount); // There should be two sections given the buffer wrapped around.
  ASSERT_EQ(bytes_to_read, range.total_bytes);
  copy_ring_data_to_buffer(&range, buf3);
  ASSERT_EQ(0, memcmp(&buf[bytes_to_overwrite], buf3, sizeof(buf) - bytes_to_overwrite));
  ASSERT_EQ(0, memcmp(buf2, &buf3[TEST_BUFFER_SIZE - bytes_to_overwrite], bytes_to_overwrite));
}

};

