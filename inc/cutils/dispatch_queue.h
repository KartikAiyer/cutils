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

#pragma once

#include <cutils/os_types.h>
#include <cutils/ts_queue.h>
#include <cutils/task.h>
#include <cutils/pool.h>
#include <cutils/signal.h>
#include <stdatomic.h>

typedef struct _dispatch_queue_t
{
  atomic_bool destroying;
  ts_queue_t *queue;
  task_t* p_task;
  pool_t *p_pool;
  char *label;
  signal_t signal;
} dispatch_queue_t;

typedef struct _dispatch_queue_create_params_t
{
  dispatch_queue_t *p_queue;
  task_create_params_t task_params;
  pool_create_params_t pool_params;
  ts_queue_create_params_t queue_params;
} dispatch_queue_create_params_t;

/**
 * @brief Used internally to pass data asynchronously between the queue worker thread and the post routines
 */
typedef struct _dispatch_queue_post_data_t
{
  dispatch_function_t fn;
  void* arg1;
  void* arg2;
}dispatch_queue_post_data_t;

#define DISPATCH_QUEUE_STORE(name)  _dispatch_queue_##name
#define DISPATCH_QUEUE_STORE_T(name)  dispatch_queue_store_##name##_t

#define DISPATCH_QUEUE_STORE_DECL(name, queue_size, stack_size)\
TASK_STATIC_STORE_DECL(dispatch_queue_##name, stack_size);\
POOL_STORE_DECL(dispatch_queue_##name, queue_size, sizeof(dispatch_queue_post_data_t), 4);\
TS_QUEUE_STORE_DECL(dispatch_queue_##name, queue_size);\
typedef struct {\
  dispatch_queue_t queue;\
}DISPATCH_QUEUE_STORE_T(name)

#define DISPATCH_QUEUE_STORE_DEF(name)\
DISPATCH_QUEUE_STORE_T(name) DISPATCH_QUEUE_STORE(name);\
TASK_STATIC_STORE_DEF(dispatch_queue_##name);\
POOL_STORE_DEF(dispatch_queue_##name);\
TS_QUEUE_STORE_DEF(dispatch_queue_##name)

#define DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, name, task_name, pri)\
memset(&(params), 0, sizeof(params));\
(params).p_queue = &DISPATCH_QUEUE_STORE(name).queue;\
TASK_STATIC_INIT_CREATE_PARAMS((params).task_params, dispatch_queue_##name, task_name, pri, NULL, NULL);\
POOL_CREATE_INIT((params).pool_params, dispatch_queue_##name);\
TS_QUEUE_STORE_CREATE_PARAMS_INIT((params).queue_params, dispatch_queue_##name)


dispatch_queue_t *dispatch_queue_create(dispatch_queue_create_params_t *create_params);

void dispatch_queue_destroy(dispatch_queue_t *p_queue);

/**
 * @brief The function will post the dispatch_function_f callback onto the dispatch queue. The client can supply up to
 * two arguments. The client is responsible for object life time maintenance of the arguments supplied.
 * @param p_queue - a successfully created dispatch_queue
 * @param fn - callback to be executed asynchronously
 * @param arg1 - client argument 1. Client maintains object lifecycle
 * @param arg2 - client argument 2. Client maintains object lifecycle
 * @return true if the action is posted on the queue. False otherwise. Indicates queue is destroyed.
 */
static inline bool dispatch_async_f(dispatch_queue_t *p_queue, dispatch_function_t fn, void *arg1, void *arg2)
{
  bool retval = false;
  if(p_queue && !atomic_load(&p_queue->destroying)){
    dispatch_queue_post_data_t *p_data = pool_alloc(p_queue->p_pool);
    CUTILS_ASSERT(p_data);
    p_data->fn = fn;
    p_data->arg1 = arg1;
    p_data->arg2 = arg2;
    CUTILS_ASSERT(ts_queue_enqueue(p_queue->queue, p_data, NO_SLEEP));
    retval = true;
  }
  return retval;
}

bool dispatch_after_f(dispatch_queue_t *p_queue,
                      uint32_t delay_ms,
                      dispatch_function_t fn,
                      void *arg1,
                      void *arg2);

dispatch_queue_timed_action_h dispatch_start_repeated_f(dispatch_queue_t *p_queue,
                                                        uint32_t initial_ms,
                                                        uint32_t reload_ms,
                                                        dispatch_function_t fn,
                                                        void *arg1,
                                                        void *arg2);

void dispatch_stop_repeated_f(dispatch_queue_timed_action_h h_action);
