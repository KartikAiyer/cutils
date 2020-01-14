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

#include <cutils/dispatch_queue.h>

static int dispatch_queue_worker(void *ctx)
{
  dispatch_queue_t *p_queue = (dispatch_queue_t *) ctx;
  while (1) {
    dispatch_queue_post_data_t *p_data = 0;
    CUTILS_ASSERT(ts_queue_dequeue(p_queue->queue, (void **) &p_data, WAIT_FOREVER));
    if ((void *) p_data == (void *) p_queue) {
      //Kill Request
      break;
    } else {
      p_data->fn(p_data->arg1, p_data->arg2);
      pool_free(p_queue->p_pool, p_data);
    }
  }
  signal_send(&p_queue->signal);
  return 0;
}

dispatch_queue_t *dispatch_queue_create(dispatch_queue_create_params_t *params)
{
  dispatch_queue_t *retval = 0;

  CUTILS_ASSERTF(params, "Provide valid Create Params");
  CUTILS_ASSERTF(params->queue_params.size, "Require a non-zero queue size");
  CUTILS_ASSERTF(params->p_queue,"Control block not provided");

  memset(params->p_queue, 0, sizeof(dispatch_queue_t));

  CUTILS_ASSERTF(signal_new(&params->p_queue->signal), "Couldn't create exit signal");

  params->p_queue->queue = ts_queue_init(&params->queue_params);
  CUTILS_ASSERTF(params->p_queue->queue, "Couldn't create thread safe queue");

  params->p_queue->p_pool = pool_create(&params->pool_params);
  CUTILS_ASSERTF(params->p_queue->p_pool, "Unable to create pool");

  params->p_queue->label = params->task_params.label;
  atomic_init(&params->p_queue->destroying, false);
  params->task_params.func = dispatch_queue_worker;
  params->task_params.ctx = params->p_queue;
  params->p_queue->p_task = task_new_static(&params->task_params);
  CUTILS_ASSERTF(params->p_queue->p_task, "Couldn't Create Task");

  task_start(params->p_queue->p_task);
  retval = params->p_queue;

  return retval;
}

void dispatch_queue_destroy(dispatch_queue_t *p_queue)
{
  if (p_queue && !atomic_flag_test_and_set(&p_queue->destroying)) {
    //We use a pointer value that is unacceptable to indicate that we want to kill the thread.
    //The thread worker function will check items popped off the queue and if this pointer value is encountered
    //It will exit the thread
    dispatch_queue_post_data_t *p_data = (dispatch_queue_post_data_t *) p_queue;
    CUTILS_ASSERTF(ts_queue_enqueue(p_queue->queue, p_data, NO_SLEEP), "Unable to queue Kill request");
    //Wait for thread to exit
    if (signal_wait(&p_queue->signal)) {
      task_destroy_static(p_queue->p_task);
      p_queue->p_task = 0;
      pool_destroy(p_queue->p_pool);
      p_queue->p_pool = 0;
      ts_queue_destroy(p_queue->queue);
      p_queue->queue = 0;
      signal_free(&p_queue->signal);
    } else {
      CUTILS_ASSERTF(0, "Failed to wait on exit signal");
    }
  }
}
#if 0
bool dispatch_after_f(dispatch_queue_t *p_queue,
                      uint32_t delay_ms,
                      dispatch_function_t fn,
                      void *arg1,
                      void *arg2)
{
  bool retval = false;

  MAGE_ASSERT(p_queue);
  MAGE_ASSERT(fn);
  if (delay_ms == NO_SLEEP) {
    retval = dispatch_async_f(p_queue, fn, arg1, arg2);
  } else {
    retval = (timer_thread_set(timer_thread_system_get(), p_queue, fn, arg1, arg2, delay_ms, 0) != 0);
  }
  return retval;
}

dispatch_queue_timed_action_h dispatch_start_repeated_f(dispatch_queue_t *p_queue,
                                                        uint32_t initial_ms,
                                                        uint32_t reload_ms,
                                                        dispatch_function_t fn,
                                                        void *arg1,
                                                        void *arg2)
{
  dispatch_queue_timed_action_h retval = 0;
  MAGE_ASSERT(p_queue);
  MAGE_ASSERT(fn);

  if(initial_ms == 0){
    dispatch_async_f(p_queue, fn, arg1, arg2);
    initial_ms = reload_ms;
  }

  if(initial_ms || reload_ms) {
    retval = timer_thread_set(timer_thread_system_get(), p_queue, fn, arg1, arg2, initial_ms, reload_ms);
  }
  return retval;
}

void dispatch_stop_repeated_f(dispatch_queue_timed_action_h h_action)
{
  if(h_action) {
    timer_thread_delete(timer_thread_system_get(), h_action);
  }
}
#endif
