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
#include <cutils/kqueue.h>
#include <cutils/c11/c11threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  cnd_t cnd;
  mutex_t mtx;
  kqueue_t queue;
  size_t count;
}ts_queue_t;

typedef KListElem ts_queue_item_t;

static inline void ts_queue_init(ts_queue_t *p_queue)
{
  if(p_queue) {
    kqueue_init(&p_queue->queue);
    CHECK_RUN(mutex_new(&p_queue->mtx), kqueue_drop_all(&p_queue->queue); return, "Failed to create Mutex");
    CHECK_RUN(!cnd_init(&p_queue->cnd),
              mutex_free(&p_queue->mtx);
              kqueue_drop_all(&p_queue->queue);
              return,
              "Failed to create Condition variable");
    p_queue->count = 0;
  }
}

static inline void ts_queue_destroy(ts_queue_t *p_queue, ts_queue_item_t *p_item)
{
  if(p_queue) {
    cnd_destroy(&p_queue->cnd);
    mutex_free(&p_queue->mtx);
    kqueue_drop_all(&p_queue->queue);
  }
}
static inline bool ts_queue_enqueue(ts_queue_t* p_queue, ts_queue_item_t* p_item)
{
  if(p_queue ) {
    mutex_lock(&p_queue->mtx, WAIT_FOREVER);
    kqueue_insert(&p_queue->queue, p_item);
    p_queue->count++;
    cnd_signal(&p_queue->cnd);
    mutex_unlock(&p_queue->mtx);
    return true;
  }
  return false;
}

static inline bool ts_queue_dequeue(ts_queue_t* p_queue, ts_queue_item_t** pp_item, uint32_t wait_ms)
{
  bool retval = false;
  bool blocking = true;
  struct timespec ts = {0};

  if(wait_ms != WAIT_FOREVER) {
    blocking = false;
    ts = { .tv_sec = wait_ms/1000, .tv_nsec = ((long)wait_ms % 1000) * 1000000};
  }
  if(p_queue && pp_item) {
    //This is always blocking because the mutex is only taken for non-blocking operations.
    //Only the condition variable wait is timed, since there is no bound on how long it will
    //take for an item to be enqueued onto the queue.
    mutex_lock(&p_queue->mtx, WAIT_FOREVER);
    if(p_queue->count > 0) {
      *pp_item = kqueue_dequeue(&p_queue->queue);
      p_queue->count--;
      retval = true;
    } else{
      int rval = 0;
      do {
        if(blocking) {
          rval = cnd_wait(&p_queue->cnd, &p_queue->mtx.mtx);
        } else {
          rval = cnd_timedwait(&p_queue->cnd, &p_queue->mtx.mtx, &ts);
        }
      }while(p_queue->count == 0 && (rval == 0 || rval != thrd_timedout));
      if(p_queue->count > 0) {
        *pp_item = kqueue_dequeue(&p_queue->queue);
        p_queue->count--;
        retval = true;
      }
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
