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
#include <cutils/mutex.h>
#include <cutils/event_flag.h>
#include <cutils/task.h>

namespace tests
{
class MutexTest : public ::testing::Test
{
protected:
  MutexTest() : m_mtx{0}
  {}

  void SetUp() override
  {
    mutex_new(&m_mtx);
  }

  void TearDown() override
  {
    mutex_free(&m_mtx);
    Test::TearDown();
  }

protected:
  mutex_t m_mtx;
};

TEST_F(MutexTest, MutexApiShouldReturnValidData)
{
  ASSERT_TRUE(mutex_unlock(&m_mtx));
  ASSERT_TRUE(mutex_lock(&m_mtx, 0));
  ASSERT_TRUE(mutex_unlock(&m_mtx));
}

TEST_F(MutexTest, MutexTimedAPIShouldWorkAsExpected)
{
  ASSERT_TRUE(mutex_lock(&m_mtx, 0));
  ASSERT_TRUE(!mutex_lock(&m_mtx, 1000));
  ASSERT_TRUE(mutex_unlock(&m_mtx));
}

class EventFlagTest : public ::testing::Test {
public:
  EventFlagTest() : m_evt{0}
  {}
protected:
  void SetUp() override
  {
    event_flag_new(&m_evt);
  }

  void TearDown() override
  {
    event_flag_free(&m_evt);
  }

protected:
  event_flag_t m_evt;

};

TEST_F(EventFlagTest, EventFlagAPI) {
  ASSERT_FALSE(event_flag_wait(&m_evt, 0x1, WAIT_OR_CLEAR, nullptr, NO_SLEEP ));
}
TEST_F(EventFlagTest, EventFlagAnswerForCorrectBits)
{
  uint32_t actual_flags = 0;
  ASSERT_TRUE(!event_flag_wait(&m_evt, 0x3, WAIT_OR_CLEAR, &actual_flags, 0));
  ASSERT_EQ(0,actual_flags);
  ASSERT_TRUE(event_flag_send(&m_evt, 0x1));
  ASSERT_TRUE(event_flag_wait(&m_evt, 0x3, WAIT_OR, &actual_flags, 0));
  ASSERT_EQ(1, actual_flags);
  ASSERT_FALSE(event_flag_wait(&m_evt, 0x3, WAIT_AND, &actual_flags, 0));
  ASSERT_EQ(1, actual_flags);
  ASSERT_TRUE(event_flag_wait(&m_evt, 0x3, WAIT_OR_CLEAR, &actual_flags, 0));
  ASSERT_EQ(1, actual_flags);
  ASSERT_FALSE(event_flag_wait(&m_evt, 0x1, WAIT_OR_CLEAR, &actual_flags, 0));
  ASSERT_FALSE(event_flag_wait(&m_evt, 0x3, WAIT_AND, &actual_flags, 0));
  ASSERT_TRUE(event_flag_send(&m_evt, 0x3));
  ASSERT_TRUE(event_flag_wait(&m_evt, 0x3, WAIT_AND_CLEAR, &actual_flags, 0));
  ASSERT_EQ(0x3, actual_flags);
  ASSERT_FALSE(event_flag_wait(&m_evt, 0x3, WAIT_OR, &actual_flags, 0));
}

TASK_STATIC_STORE_DECL(test_task,16 * 1024);
TASK_STATIC_STORE_DEF(test_task);

class TaskTest : public ::testing::Test {
public:
  TaskTest() : m_flag{0}, m_value{0}, m_mutex{0}, m_task{0}
  {}
protected:
  void SetUp() override
  {
    m_value = 0;
    event_flag_new(&m_flag);
    mutex_new(&m_mutex);
  }

  void TearDown() override
  {
    event_flag_free(&m_flag);
    mutex_free(&m_mutex);
  }
  static void test_thread_function(void *arg);
  static void prempt_test_thread_hi(void *arg);
  static void prempt_test_thread_mid(void *arg);
public:
  void set_value(uint32_t value) {
    m_value = value;
  }
  uint32_t get_value() {
    return m_value;
  }
protected:
  task_t m_task;
  event_flag_t m_flag;
  mutex_t m_mutex;
  uint32_t m_value;
};

void TaskTest::test_thread_function(void *arg)
{
  auto* p_task = (TaskTest*) arg;
  p_task->set_value(1);
}

TEST_F(TaskTest, TaskApiTest)
{
  task_create_params_t params;
  TASK_STATIC_INIT_CREATE_PARAMS(params, test_task, (char*)"Test Thread", CUTILS_TASK_PRIORITY_MEDIUM, test_thread_function,
                                 (void*)this);
  task_t *p_task = task_new_static(&params);
  ASSERT_TRUE(p_task);
  task_start(p_task);
  task_sleep(10);
  ASSERT_EQ(1, this->get_value());
  task_destroy_static(p_task);
}

void TaskTest::prempt_test_thread_hi(void *arg)
{
  auto * p_test = (TaskTest*)arg;
  mutex_lock(&p_test->m_mutex, WAIT_FOREVER);
  p_test->m_value= 20;
  mutex_unlock(&p_test->m_mutex);
  event_flag_send(&p_test->m_flag, 0x2);
}

void TaskTest::prempt_test_thread_mid(void *arg)
{
  uint32_t val = 0;
  bool keepRunning = true;
  auto *p_test = (TaskTest*)arg;

  while (keepRunning) {
    mutex_lock(&p_test->m_mutex, WAIT_FOREVER);
    val = p_test->m_value;
    mutex_unlock(&p_test->m_mutex);
    task_sleep(1);
    keepRunning = val == 0;
  }
  event_flag_send(&p_test->m_flag, 0x1);
}

TASK_STATIC_STORE_DECL(thread_mid, 2048);
TASK_STATIC_STORE_DECL(thread_hi, 2048);

static TASK_STATIC_STORE_DEF(thread_mid);
static TASK_STATIC_STORE_DEF(thread_hi);

TEST_F(TaskTest, BasicPremption)
{
  task_t *p_task_mid = 0;
  task_t *p_task_hi = 0;
  task_create_params_t params = {0};
  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 thread_mid,
                                 (char*)"ThreadMid",
                                 CUTILS_TASK_PRIORITY_MEDIUM,
                                 TaskTest::prempt_test_thread_mid,
                                 this);
  p_task_mid = task_new_static(&params);
  ASSERT_TRUE(p_task_mid);


  TASK_STATIC_INIT_CREATE_PARAMS(params,
                                 thread_hi,
                                 (char*)"ThreadHi",
                                 CUTILS_TASK_PRIORITY_MID_HIGH,
                                 prempt_test_thread_hi,
                                 this);
  p_task_hi = task_new_static(&params);
  ASSERT_TRUE(p_task_hi);
  task_start(p_task_mid);
  task_start(p_task_hi);
  ASSERT_TRUE(event_flag_wait(&this->m_flag, 0x3, WAIT_AND_CLEAR, nullptr, 200));
  ASSERT_EQ(20, this->m_value);
  mutex_free(&this->m_mutex);
  event_flag_free(&this->m_flag);
  task_destroy_static(p_task_mid);
  task_destroy_static(p_task_hi);
}
}