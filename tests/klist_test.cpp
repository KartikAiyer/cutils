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

#include <gtest/gtest.h>
#include <cutils/klist.h>

namespace tests {
class KlistTest : public ::testing::Test {
public:
  KlistTest() = default;
  ~KlistTest() override = default;
  void SetUp() override {}
  void TearDown() override {}
};

struct TestObj {
  KListElem elem;
  uint32_t val;
};

TEST_F(KlistTest, MustInsert) {
  TestObj bufs[10];
  memset(bufs, 0, sizeof(bufs));
  KListHead *p_head = nullptr;
  for(uint32_t i = 0 ; i < sizeof(bufs)/sizeof(bufs[0]); i++) {
    bufs[i].val = 10 + i;
    KLIST_HEAD_PREPEND(p_head, &bufs[i].elem);
  }

  auto count_up_f = [](KListElem* elem, ...) -> void {
    va_list args;
    va_start(args, elem);
    uint32_t *p_count = va_arg(args, uint32_t*);
    (*p_count)++;
    va_end(args);
  };
  uint32_t num_of_elements = 0;
  KLIST_HEAD_FOREACH(p_head, count_up_f, &num_of_elements);
  ASSERT_EQ(sizeof(bufs)/sizeof(bufs[0]), num_of_elements );

  uint32_t i = 1;
  while(p_head) {
    TestObj *elem = 0;
    KLIST_HEAD_POP(p_head, elem);
    ASSERT_EQ(10 + (num_of_elements - i), elem->val);
    i++;
  }
}

}
