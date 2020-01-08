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
#pragma once

#include <cutils/os_types.h>
#include <cutils/c11/c11threads.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _mutex_t
{
  mtx_t mtx;
} mutex_t;

static inline bool mutex_new(mutex_t *mutex)
{
  bool retval = false;
  if (mutex &&
      !mtx_init(&mutex->mtx, mtx_timed)) {
    retval = true;
  }
  return retval;
}

static inline void mutex_free(mutex_t *mutex)
{
  if (mutex) {
    mtx_destroy(&mutex->mtx);
  }
}

static inline bool mutex_lock(mutex_t *mutex, uint32_t wait_ms)
{
  bool retval = false;
  if (mutex) {
    if(!wait_ms) {
      retval = !mtx_trylock(&mutex->mtx);
    } else {
      struct timespec tm = {0};
      clock_gettime(CLOCK_REALTIME, &tm);
      tm.tv_sec += wait_ms/1000;
      tm.tv_nsec += 1000000 * ( wait_ms % 1000);
      retval = (!mtx_timedlock(&mutex->mtx, &tm));
    }
  }
  return retval;
}

static inline bool mutex_unlock(mutex_t *mutex)
{
  bool retval = false;
  if (mutex) {
    retval = (!mtx_unlock(&mutex->mtx));
  }
  return retval;
}

#ifdef __cplusplus
};
#endif
