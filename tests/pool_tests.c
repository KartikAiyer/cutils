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

#include <cutils/pool.h>
#include <cutils/task.h>
#include <cutils/klist.h>
#include <embUnit/embUnit.h>
#include <cutils/logger.h>
#include <stdlib.h>

#define _pool_test1_ALIGN       ( 64 )
#define _pool_test1_QUEUE_SIZE ( 4 )
#define _pool_test2_ALIGN       ( 32 )
#define _pool_test2_QUEUE_SIZE ( 8 )
#define _pool_test3_ALIGN       ( 1 )
#define _pool_test3_QUEUE_SIZE ( 16 )

typedef struct _test_allocation_t
{
  KListElem elem;
  uint32_t dummy;
} test_allocation_t;

POOL_STORE_DECL(pool_test1, _pool_test1_QUEUE_SIZE, sizeof(test_allocation_t), _pool_test1_ALIGN);
POOL_STORE_DECL(pool_test2, _pool_test2_QUEUE_SIZE, sizeof(test_allocation_t), _pool_test2_ALIGN);
POOL_STORE_DECL(pool_test3, _pool_test3_QUEUE_SIZE, sizeof(test_allocation_t), _pool_test3_ALIGN);
POOL_STORE_DEF(pool_test1);
POOL_STORE_DEF(pool_test2);
POOL_STORE_DEF(pool_test3);

//The only reason why this actually cylces through all the memory entries
//is because the pool uses a queue (FIFO) to maintain the free buffers.
#define TEST_A_POOL_WITH_CREATE_PARAMS(name)\
{\
  pool_create_params_t params;\
  POOL_CREATE_INIT( params, name );\
  pool_t* p_pool = pool_create( &params );\
  TEST_ASSERT_MESSAGE( p_pool != 0, "Couldn't create Static Pool" );\
  TEST_ASSERT_EQUAL_INT( _##name##_QUEUE_SIZE, p_pool->num_of_elements );\
  KListHead *head = NULL;\
  for( size_t i = 0; i < p_pool->num_of_elements; i++ ){ \
    void* alloc = pool_alloc( p_pool );\
    TEST_ASSERT_MESSAGE(alloc, "Couldn't allocate from pool" );\
    TEST_ASSERT_MESSAGE(((size_t)alloc & (_##name##_ALIGN - 1)) == 0, "Allocation not aligned");\
    memset(alloc, 0, p_pool->element_size);\
    pool_header_t* p_header = (pool_header_t*)((uint8_t*)alloc - p_pool->offset_header_from_data);\
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_HEADER_SANITY == p_header->sanity, "Header sanity is not valid");\
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_TRAILER_SANITY == *((uint32_t*)(alloc + p_pool->element_size)), "Footer sanity does not match"  );\
    KLIST_HEAD_PREPEND(head, alloc);\
  }\
  while(head) {\
    void* alloc = 0;\
    KLIST_HEAD_POP(head, alloc);\
    TEST_ASSERT(alloc);\
    pool_free(p_pool, alloc);\
  }\
  pool_destroy(p_pool);\
}

static void pool_static_should_create_with_params_aligned_allocations_test1(void)
{
  TEST_A_POOL_WITH_CREATE_PARAMS(pool_test1);
}

static void pool_static_should_create_with_params_aligned_allocations_test2(void)
{
  TEST_A_POOL_WITH_CREATE_PARAMS(pool_test2);
}

static void pool_static_should_create_with_params_aligned_allocations_test3(void)
{
  TEST_A_POOL_WITH_CREATE_PARAMS(pool_test3);
}

typedef struct _ref_count_test_t
{
  uint32_t total_count;
} ref_count_test_t;

static ref_count_test_t s_test_data = {0};

static void test_destructor_f(void *mem, void *private)
{
  TEST_ASSERT_MESSAGE(s_test_data.total_count == 0, "Destructor called before References removed");
  s_test_data.total_count = (uint32_t) -1;
}

static void pool_test_ref_count(void)
{
  pool_create_params_t params;
  POOL_CREATE_INIT(params, pool_test3);
  pool_t *p_pool = pool_create(&params);
  TEST_ASSERT_MESSAGE(p_pool, "Couldn't create Static Pool");
  TEST_ASSERT_EQUAL_INT(_pool_test3_QUEUE_SIZE, p_pool->num_of_elements);
  KListHead *head = 0;
  for (uint32_t i = 0; i < p_pool->num_of_elements; i++) {
    void *alloc = pool_alloc(p_pool);
    TEST_ASSERT_MESSAGE(alloc, "Couldn't allocate from pool");
    TEST_ASSERT_MESSAGE(((size_t) alloc & (_pool_test3_ALIGN - 1)) == 0, "Allocation not aligned");
    pool_header_t *p_header = (pool_header_t *) ((uint8_t *) alloc -
                                                 p_pool->offset_header_from_data);
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_HEADER_SANITY == p_header->sanity, "Header sanity is not valid");\
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_TRAILER_SANITY == *((uint32_t *) (alloc + p_pool->element_size)),
                        "Footer sanity does not match");\
    memset(alloc, 0, p_pool->element_size);
    s_test_data.total_count++;
    TEST_ASSERT_MESSAGE(atomic_load(&p_header->retain_count) == 1, "Expected a retain count of 1");
    pool_retain(p_pool, alloc);
    pool_set_destructor(p_pool, alloc, test_destructor_f, NULL);
    KLIST_HEAD_PREPEND(head, alloc);
    TEST_ASSERT_MESSAGE(atomic_load(&p_header->retain_count) == 2, "Expected retain count of 2");
  }
  uint32_t count = 0;
  while (head) {
    void *alloc = 0;

    count++;
    KLIST_HEAD_POP(head, alloc);
    TEST_ASSERT_NOT_NULL(alloc);
    pool_header_t *p_header = (pool_header_t *) ((uint8_t *) alloc -
                                                 p_pool->offset_header_from_data);
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_HEADER_SANITY == p_header->sanity, "Header sanity is not valid");\
    TEST_ASSERT_MESSAGE(POOL_ELEMENT_TRAILER_SANITY == *((uint32_t *) (alloc + p_pool->element_size)),
                        "Footer sanity does not match");\
    pool_free(p_pool, alloc);
    TEST_ASSERT_MESSAGE(atomic_load(&p_header->retain_count) == 1, "Expected retain count to drop to 1");
    s_test_data.total_count = 0;
    pool_free(p_pool, alloc);
    TEST_ASSERT_MESSAGE(atomic_load(&p_header->retain_count) == 0, "Expected retain count to drop to 0");
  }
  TEST_ASSERT_EQUAL_INT(p_pool->num_of_elements, count);
  TEST_ASSERT_MESSAGE(s_test_data.total_count == (uint32_t) -1, "Unexpected final value. Destructor not called");
  pool_destroy(p_pool);
}

static void setUp(void)
{
  memset(&s_test_data, 0, sizeof(s_test_data));
}

static void tearDown(void)
{}

#if 0
/**
 * @brief The following unit test attempts to test the reference counting of the pool across a number of threads
 * It creates a high priority launcher thread whose job is to create a large number of test threads of lower priority
 * than the launcher and then exit. The Main test will create a test pool, create an allocation from the test pool.
 * It will then start the launcher and wait for it to complete. It then waits for all test threads to complete and
 * finally deallocates the allocation and confirms that the destructor is called.
 */
#define NUM_OF_THREADS      (1000)
#define MAX_TEST_TASK_SLEEP (10)

TASK_STATIC_STORE_DECL(launcher_task, 4096);
TASK_STATIC_STORE_DECL(retainer_task, 1024);

typedef enum
{
  POOL_RETAIN_TEST_RUNNING,
  POOL_RETAIN_TEST_FAIL_ALLOC,
  POOL_RETAIN_TEST_FAIL_CREATE,
  POOL_RETAIN_TEST_SUCCESS
} pool_retain_test_results_e;

typedef struct
{
  pool_t *p_pool;
  void *alloc;
  TASK_STATIC_STORE_T(launcher_task) *p_launcher_store;
  TASK_STATIC_STORE_T(retainer_task) *retainerStores[NUM_OF_THREADS];
  signal_t launcher_exit;
  pool_retain_test_results_e result;
  atomic_uint complete_count;
} pool_retain_test_data_t;
static pool_retain_test_data_t s_test_data2 = {0};

static void cleanup_launcher(pool_retain_test_data_t *p_test_data)
{
  task_t *p_launcher_task = &p_test_data->p_launcher_store->tsk;
  task_destroy_static(p_launcher_task);
  AmbaKAL_BytePoolFree(p_test_data->p_launcher_store);
  p_test_data->p_launcher_store = 0;
}

static void cleanup_retainers(pool_retain_test_data_t *p_test_data)
{
  for (uint32_t i = 0; i < NUM_OF_THREADS; i++) {
    if (p_test_data->retainerStores[i]) {
      task_t *p_task = &p_test_data->retainerStores[i]->tsk;
      task_destroy_static(p_task);
      AmbaKAL_BytePoolFree(p_test_data->retainerStores[i]);
      p_test_data->retainerStores[i] = 0;
    }
  }
}

static void pool_retainer_action(void *ctx)
{
  pool_retain_test_data_t *p_test_data = (pool_retain_test_data_t *) ctx;
  pool_retain(p_test_data->p_pool, p_test_data->alloc);
  uint32_t timeout = (uint32_t) rand() % MAX_TEST_TASK_SLEEP;
  task_sleep(timeout);
  pool_free(p_test_data->p_pool, p_test_data->alloc);
  atomic_fetch_add_explicit(&p_test_data->complete_count, 1, memory_order_relaxed);
}

static void launcher_action(void *ctx)
{
  pool_retain_test_data_t *p_test_data = (pool_retain_test_data_t *) ctx;

  for (uint32_t i = 0; i < NUM_OF_THREADS; i++) {
    if (AmbaKAL_BytePoolAllocate(&AmbaBytePool_Cached,
                                 (void **) &p_test_data->retainerStores[i],
                                 sizeof(TASK_STATIC_STORE_T(retainer_task)),
                                 AMBA_KAL_NO_WAIT) == OK) {
      task_create_params_t retainer_params = {
          .task = &p_test_data->retainerStores[i]->tsk,
          .stack = p_test_data->retainerStores[i]->stack,
          .stack_size = sizeof(p_test_data->retainerStores[i]->stack),
          .label = "RetainerTask",
          .priority = RYLO_TASK_PRIORITY_MID_LO,
          .func = pool_retainer_action,
          .ctx = p_test_data
      };
      memset(retainer_params.task, 0, sizeof(task_t));
      if (!task_new_static(&retainer_params)) {
        p_test_data->result = POOL_RETAIN_TEST_FAIL_CREATE;
        cleanup_retainers(p_test_data);
        break;
      }
      task_start(retainer_params.task);
    } else {
      cleanup_retainers(p_test_data);
      p_test_data->result = POOL_RETAIN_TEST_FAIL_ALLOC;
      break;
    }
  }
  signal_send(&p_test_data->launcher_exit);
}

static void destructor_fn(void *mem, void *private)
{
  pool_retain_test_data_t *p_test_data = (pool_retain_test_data_t *) private;
  p_test_data->result = POOL_RETAIN_TEST_SUCCESS;
}

POOL_STORE_DECL(retain_test_pool, 1, sizeof(uint32_t), sizeof(uint32_t));
POOL_STORE_DEF(retain_test_pool);

static void pool_multi_thread_alloc_test(void)
{
  task_t *launcher_task = 0;
  srand(task_get_ticks());

  memset(&s_test_data2, 0, sizeof(s_test_data2));

  if (signal_new(&s_test_data2.launcher_exit)) {
    pool_create_params_t pool_params = {0};
    POOL_CREATE_INIT(pool_params, retain_test_pool);
    s_test_data2.p_pool = pool_create(&pool_params);
    if (!s_test_data2.p_pool) {
      signal_free(&s_test_data2.launcher_exit);
      TEST_ASSERT_MESSAGE(0, "Failed to create test pool");
    }
    // Create an Allocation from the pool
    s_test_data2.alloc = pool_alloc(s_test_data2.p_pool);
    if (!s_test_data2.p_pool) {
      pool_destroy(s_test_data2.p_pool);
      signal_free(&s_test_data2.launcher_exit);
      TEST_ASSERT_MESSAGE(0, "Failed to create an allocation from the pool");
    }
    if (AmbaKAL_BytePoolAllocate(&AmbaBytePool_Cached,
                                 (void **) &s_test_data2.p_launcher_store,
                                 sizeof(TASK_STATIC_STORE_T(launcher_task)),
                                 AMBA_KAL_NO_WAIT) == OK) {
      task_create_params_t launcher_params = {
          .task = &s_test_data2.p_launcher_store->tsk,
          .stack = s_test_data2.p_launcher_store->stack,
          .stack_size = sizeof(s_test_data2.p_launcher_store->stack),
          .priority = RYLO_TASK_PRIORITY_HIGHEST,
          .label = "LauncherTask",
          .func = launcher_action,
          .ctx = &s_test_data2
      };
      launcher_task = task_new_static(&launcher_params);
      if (!launcher_task) {
        AmbaKAL_BytePoolFree(s_test_data2.p_launcher_store);
        pool_free(s_test_data2.p_pool, s_test_data2.alloc);
        pool_destroy(s_test_data2.p_pool);
        signal_free(&s_test_data2.launcher_exit);
        TEST_ASSERT_MESSAGE(0, "Failed to start Launcher Task");
      }
      task_start(launcher_task);
    } else {
      pool_free(s_test_data2.p_pool, s_test_data2.alloc);
      pool_destroy(s_test_data2.p_pool);
      signal_free(&s_test_data2.launcher_exit);
      TEST_ASSERT_MESSAGE(0, "Failed to allocate thread store for launcher task.")
    }
  } else {
    signal_free(&s_test_data2.launcher_exit);
    TEST_ASSERT_MESSAGE(0, "Failed to allocate basic resources for test");
  }
  //Everything was setup correctly. If anything failed in setup, the test would return above this

  signal_wait(&s_test_data2.launcher_exit);
  //Launcher exited. Perform tests
  cleanup_launcher(&s_test_data2);
  //Check all threads are done
  bool success = false;
  for (uint32_t i = 0; i < NUM_OF_THREADS; i++) {
    uint32_t count = atomic_load_explicit(&s_test_data2.complete_count, memory_order_acquire);
    if (count == NUM_OF_THREADS) {
      success = true;
      break;
    }
    task_sleep(MAX_TEST_TASK_SLEEP);
  }
  if (!success) {
    CLOG("Retainer Thread Failed to complete. Count = %d",
              atomic_load_explicit(&s_test_data2.complete_count, memory_order_acquire));
    cleanup_retainers(&s_test_data2);
    pool_destroy(s_test_data2.p_pool);
    signal_free(&s_test_data2.launcher_exit);
    TEST_ASSERT_MESSAGE(0, "Retain tasks failed to complete");

  }
  pool_set_destructor(s_test_data2.p_pool, s_test_data2.alloc, destructor_fn, &s_test_data2);
  //Free the allocation. This should destroy it
  pool_free(s_test_data2.p_pool, s_test_data2.alloc);
  cleanup_retainers(&s_test_data2);
  pool_destroy(s_test_data2.p_pool);
  signal_free(&s_test_data2.launcher_exit);
  TEST_ASSERT(s_test_data2.result == POOL_RETAIN_TEST_SUCCESS);
}
#endif
TestRef pool_get_tests()
{
  EMB_UNIT_TESTFIXTURES(fixtures) {
      new_TestFixture("Pool is created with proper alignments using create params 1",
                      pool_static_should_create_with_params_aligned_allocations_test1),
      new_TestFixture("Pool is created with proper alignments using create params 2",
                      pool_static_should_create_with_params_aligned_allocations_test2),
      new_TestFixture("Pool is created with proper alignments using create params 3",
                      pool_static_should_create_with_params_aligned_allocations_test3),
      new_TestFixture("Pool allocations can be reference counted", pool_test_ref_count),
//      new_TestFixture("Pool allocations can be referenced counted across many threads", pool_multi_thread_alloc_test)
  };
  EMB_UNIT_TESTCALLER(pool_basic_test, "PoolBasicTests", setUp, tearDown, fixtures);
  return (TestRef) &pool_basic_test;
}

int main()
{
  TestRunner_start();
  {
    TestRunner_runTest(pool_get_tests());
  }
  TestRunner_end();
  return 0;
}