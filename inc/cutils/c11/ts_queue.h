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


#ifndef CUTILS_C11_TS_QUEUE_H
#define CUTILS_C11_TS_QUEUE_H

#include <cutils/mutex.h>
#include <cutils/c11/c11threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  cnd_t cnd;
  cnd_t full_cnd;
  mutex_t mtx;
  void** pp_ptr_array;
  size_t size;
  size_t count;
  atomic_ulong head;
  atomic_ulong tail;
}ts_queue_t;

typedef KListElem ts_queue_item_t;

#define TS_QUEUE_STORE(name)      _ts_queue_store_##name
#define TS_QUEUE_STORE_T(name)    _ts_queue_store_##name##_t
#define TS_QUEUE_STORE_DECL(name, size) \
static size_t _ts_queue_store_num_elements_##name = size;\
typedef struct {\
  void* ptr_array[size];\
  ts_queue_t queue;\
}TS_QUEUE_STORE_T(name)

#define TS_QUEUE_STORE_DEF(name) \
static TS_QUEUE_STORE_T(name) TS_QUEUE_STORE(name)

typedef struct {
  ts_queue_t* p_queue;
  void **ptr_array;
  size_t size;
}ts_queue_create_params_t;

#define TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, name) \
memset(&(params), 0, sizeof((params)));\
(params).p_queue = &TS_QUEUE_STORE(name).queue;\
(params).ptr_array = TS_QUEUE_STORE(name).ptr_array;\
(params).size = _ts_queue_store_num_elements_##name

static inline ts_queue_t *ts_queue_init(ts_queue_create_params_t *params)
{
  ts_queue_t *retval = 0;
  if(params && params->p_queue && params->ptr_array) {
    params->p_queue->pp_ptr_array = params->ptr_array;
    params->p_queue->size = params->size;
    CHECK_RUN(params->p_queue->size > 0 && (params->p_queue->size & (params->p_queue->size -1)) == 0,
        return retval, "Queue Size must be a power of 2");
    CHECK_RUN(mutex_new(&params->p_queue->mtx), return retval, "Failed to create Mutex");
    CHECK_RUN(!cnd_init(&params->p_queue->cnd) && !cnd_init(&params->p_queue->full_cnd),
              mutex_free(&params->p_queue->mtx);
              return retval,
              "Failed to create Condition variable");
    params->p_queue->count = 0;
    params->p_queue->head = params->p_queue->tail = 0;
    retval = params->p_queue;
  }
  return retval;
}

static inline void ts_queue_destroy(ts_queue_t *p_queue)
{
  if(p_queue) {
    cnd_destroy(&p_queue->cnd);
    mutex_free(&p_queue->mtx);
  }
}

static inline bool ts_queue_enqueue(ts_queue_t* p_queue, void* p_item, uint32_t wait_ms)
{
  struct timespec ts = {0};
  bool retval = false;

  clock_gettime(CLOCK_REALTIME, &ts);
  if(wait_ms != WAIT_FOREVER) {
    ts.tv_sec += wait_ms/1000;
    ts.tv_nsec += ((long)wait_ms % 1000) * 1000000;
  }
  if(p_queue && p_item) {
    mutex_lock(&p_queue->mtx, WAIT_FOREVER);
    int rval = 0;
    while(p_queue->tail + p_queue->size <= p_queue->head && rval != thrd_timedout) {
      rval = cnd_timedwait(&p_queue->full_cnd, &p_queue->mtx.mtx, &ts);
    }
    if(rval != thrd_timedout) {
      p_queue->pp_ptr_array[atomic_fetch_add(&p_queue->head, 1) & (p_queue->size - 1)] = p_item;
      p_queue->count++;
      cnd_signal(&p_queue->cnd);
      retval = true;
    }
    mutex_unlock(&p_queue->mtx);
    return retval;
  }
  return retval;
}

static inline bool ts_queue_dequeue(ts_queue_t* p_queue, void** pp_item, uint32_t wait_ms)
{
  bool retval = false;
  struct timespec ts = {0};
  clock_gettime(CLOCK_REALTIME, &ts);

  if(wait_ms != WAIT_FOREVER) {
    ts.tv_sec += wait_ms/1000;
    ts.tv_nsec += ((long)wait_ms % 1000) * 1000000;
  }
  if(p_queue && pp_item) {
    int rval = 0;
    //This is always blocking because the mutex is only taken for non-blocking operations.
    //Only the condition variable wait is timed, since there is no bound on how long it will
    //take for an item to be enqueued onto the queue.
    mutex_lock(&p_queue->mtx, WAIT_FOREVER);
    while(p_queue->tail >= p_queue->head && rval != thrd_timedout) {
      rval = cnd_timedwait(&p_queue->cnd, &p_queue->mtx.mtx, &ts);
    }
    if(rval != thrd_timedout) {
      *pp_item = p_queue->pp_ptr_array[atomic_fetch_add(&p_queue->tail, 1) & (p_queue->size - 1)];
      p_queue->count--;
      cnd_signal(&p_queue->full_cnd);
      retval = true;
    }
    mutex_unlock(&p_queue->mtx);
  }
  return retval;
}

static inline size_t ts_queue_get_count(ts_queue_t* p_queue)
{
  return p_queue->count;
}

#ifdef __cplusplus
};
#endif

#endif //CUTILS_C11_TS_QUEUE_H
