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

#include <gtest/gtest.h>
#include <cutils/kqueue.h>
#include <cutils/ts_queue.h>
#include <cutils/task.h>
#include <cutils/free_list.h>
#include <functional>
#include <ctime>

namespace tests {
typedef struct {
  KListElem elem; //Necessary header item
  uint32_t data; //Client Specific data
}free_list_test_unit_t;

FREE_LIST_STORE_DECL(test_free_list, free_list_test_unit_t, 10);
FREE_LIST_STORE_DEF(test_free_list);

class free_list_test : public  ::testing::Test {
protected:
  void SetUp() override
  {
    free_list_create_params_t params = { 0 };
    FREE_LIST_STORE_CREATE_PARAMS_INIT(params, test_free_list);
    m_list = free_list_init(&params);
  }

  void TearDown() override
  {
  }
  free_list_t *m_list;
};

TEST_F(free_list_test, can_create_free_list_with_appropriate_size)
{
  ASSERT_EQ(GetArraySize(FREE_LIST_STORE(test_free_list).store), m_list->item_count);
  ASSERT_EQ(GetArraySize(FREE_LIST_STORE(test_free_list).store), m_list->current_available);
}
TEST_F(free_list_test, can_allocated_as_many_as_available)
{
  KListHead* p_head = 0;
  KListElem* p_alloc = 0;
  for(uint32_t i = 0; i < m_list->item_count; i++) {
    auto item = (free_list_test_unit_t*)free_list_get(m_list);
    ASSERT_TRUE(item);
    item->data = i + 1;
    KLIST_HEAD_PREPEND(p_head, item);
  }
  ASSERT_EQ(0, m_list->current_available);
  while(p_head) {
    KListElem* list_item = nullptr;
    KLIST_HEAD_POP(p_head, list_item);
    ASSERT_TRUE(list_item);
    free_list_put(m_list, list_item);
  }
  ASSERT_EQ(GetArraySize(FREE_LIST_STORE(test_free_list).store), m_list->current_available);
}

class kqueue_test : public ::testing::Test {

protected:
  void SetUp() override
  {
    kqueue_init(&m_queue);
  }

  void TearDown() override
  {
    kqueue_drop_all(&m_queue);
  }

protected:
  kqueue_t m_queue;

};

TEST_F(kqueue_test, queue_should_return_count_of_zero_for_empty_queue) {
  ASSERT_EQ(0, kqueue_get_item_count(&m_queue));
}

TEST_F(kqueue_test, queue_should_insert_and_update_count) {
  struct {
    KListElem elem;
    int val;
  }item = { 0 };
  kqueue_insert(&m_queue, &item.elem);
  ASSERT_EQ(1, kqueue_get_item_count(&m_queue));
}

TEST_F(kqueue_test, queue_should_dequeue_as_expected) {
  struct
  {
    KListElem elem;
    int val;
  } item = { 0 };
  kqueue_insert(&m_queue, &item.elem);
  auto popped_item = kqueue_dequeue(&m_queue);
  ASSERT_EQ(0, kqueue_get_item_count(&m_queue));
  ASSERT_EQ(popped_item, &item.elem);
}

TEST_F(kqueue_test, queue_should_maintain_fifo_order)
{
  struct test_item_t {
    KListElem elem;
    int val;
  }items[10] = {0};
  for(uint32_t i = 0; i < GetArraySize(items); i++) {
    items[i].val = i+1;
    kqueue_insert(&m_queue, &items[i].elem);
  }

  for(uint32_t i = 0; i < GetArraySize(items); i++) {
    auto popped_item = kqueue_dequeue(&m_queue);
    ASSERT_EQ(&items[i].elem, popped_item);
    ASSERT_EQ(1 + i, ((test_item_t*)popped_item)->val);
  }
}

TASK_STATIC_STORE_DECL(ts_queue_producer, 16 * 1024);
TASK_STATIC_STORE_DEF(ts_queue_producer);

TASK_STATIC_STORE_DECL(ts_queue_consumer, 16 * 1024);
TASK_STATIC_STORE_DEF(ts_queue_consumer);

class ts_queue_test : public ::testing::Test
{
public:
  enum test_error_code_e
  {
    Success = 0,
    ConsumerFailed = 1,
    ProducerFailed = 2
  };
protected:
  struct task_data_t
  {
    KListElem elem;
    uint32_t val = 0;
    std::function<void(task_data_t *)> fn;
  };

  FREE_LIST_STORE_DECL(ts_queue_prod_cons_free_list, ts_queue_test::task_data_t, 10);
  FREE_LIST_STORE_DEF(ts_queue_prod_cons_free_list);

  void SetUp() override
  {
    free_list_create_params_t free_list_params = { 0};
    FREE_LIST_STORE_CREATE_PARAMS_INIT(free_list_params, ts_queue_prod_cons_free_list);
    m_data_store = free_list_init(&free_list_params);
    m_cons_task = m_prod_task = nullptr;
    ts_queue_init(&m_queue);
  }

  void TearDown() override
  {
    stop_tasks();
  }

  void start_tasks()
  {
    task_create_params_t params = {0};

    TASK_STATIC_INIT_CREATE_PARAMS(params,
                                   ts_queue_producer,
                                   "producer",
                                   CUTILS_TASK_PRIORITY_MEDIUM,
                                   prod_task,
                                   this);
    m_prod_task = task_new_static(&params);
    TASK_STATIC_INIT_CREATE_PARAMS(params,
                                   ts_queue_consumer,
                                   "consumer",
                                   CUTILS_TASK_PRIORITY_MEDIUM,
                                   cons_task,
                                   this);
    m_cons_task = task_new_static(&params);
  }
  void stop_tasks()
  {
    kill_consumer();
    if(m_prod_task) {
      task_destroy_static(m_prod_task);
      m_prod_task = nullptr;
    }
    if(m_cons_task) {
      task_destroy_static(m_cons_task);
      m_cons_task = nullptr;
    }
  }
  inline task_data_t* alloc_data()
  {
    auto retval = (task_data_t*) free_list_get(m_data_store);
    if(retval) {
      memset(retval, 0, sizeof(task_data_t));
    }
    return retval;
  }
  inline void free_data(task_data_t* data)
  {
    if(data) {
      free_list_put(m_data_store, &data->elem);
    }
  }
  bool post_data(task_data_t *data)
  {
    bool retval = false;
    if(data) {
      retval = ts_queue_enqueue(&m_queue, &data->elem);
    }
    return retval;
  }
  void kill_consumer()
  {
    auto data = alloc_data();
    if(data) {
      data->val = 9999;
      post_data(data);
    }
  }
  static int prod_task(void* ctx)
  {
    ts_queue_test *p_test = (ts_queue_test*)ctx;
    return 0;
  }
  static int cons_task(void* ctx) {
    ts_queue_item_t *item = 0;
    ts_queue_test *p_test = (ts_queue_test*)ctx;

    while(ts_queue_dequeue(&p_test->m_queue, &item, 2000))
    {
      auto test_data = (task_data_t*)item;
      if(test_data->val == 9999) {
        p_test->free_data(test_data);
        return 0;
      }
      if(test_data->fn) {
        test_data->fn(test_data);
      }
    }
    p_test->m_error_code = ConsumerFailed;
    return -1;
  }

  ts_queue_t m_queue;
  task_t *m_prod_task;
  task_t *m_cons_task;
  free_list_t *m_data_store;
  test_error_code_e m_error_code;
};

TEST_F(ts_queue_test, ShouldReportZeroCountForEmptyQueue)
{
  ASSERT_EQ(0, ts_queue_get_count(&m_queue));
}
TEST_F(ts_queue_test, ShouldBlockOnEmptyQueue)
{
  ts_queue_item_t *data = 0;
  auto c_start = std::clock();

  ASSERT_FALSE(ts_queue_dequeue(&m_queue, &data, 2000));
  auto c_end = std::clock();
  auto time = c_end - c_start;
  ASSERT_LT(999,  time);
}
TEST_F(ts_queue_test, ProducerConsumerShouldRun)
{
  start_tasks();
  task_data_t data = { .val = 0, .fn = [](task_data_t* p_data) {
    p_data->val = 10;
  }};
  ASSERT_TRUE(ts_queue_enqueue(&m_queue, &data.elem));
  stop_tasks();
  ASSERT_EQ(10, data.val);
}

}


