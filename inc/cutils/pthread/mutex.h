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
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mutex_t
{
  pthread_mutex_t mtx;
} mutex_t;

static inline bool mutex_new(mutex_t *mutex)
{
  bool res = false;
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);

  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);

  if(0) {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  }

  res = (pthread_mutex_init(&mutex->mtx, &attr) == 0);
  pthread_mutexattr_destroy(&attr);
  return res;
}

static inline void mutex_free(mutex_t *mutex)
{
  if (mutex) {
    pthread_mutex_destroy(&mutex->mtx);
  }
}

static inline bool mutex_lock(mutex_t *mutex, uint32_t wait_ms)
{
  bool retval = false;
  if (mutex) {
    if(!wait_ms) {
      retval = !pthread_mutex_trylock(&mutex->mtx);
    } else {
      struct timespec tm = {0};
      clock_gettime(CLOCK_REALTIME, &tm);
      tm.tv_sec += wait_ms/1000;
      tm.tv_nsec += 1000000 * ( wait_ms % 1000);
      retval = (!pthread_mutex_timedlock(&mutex->mtx, &tm));
    }
  }
  return retval;
}

static inline bool mutex_unlock(mutex_t *mutex)
{
  bool retval = false;
  if (mutex) {
    retval = (!pthread_mutex_unlock(&mutex->mtx));
  }
  return retval;
}

#ifdef __cplusplus
}
#endif