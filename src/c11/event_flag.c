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

#include <cutils/event_flag.h>


bool event_flag_new(event_flag_t *p_flags)
{
  bool retval = false;
  if (p_flags &&
      !mtx_init(&p_flags->mtx, mtx_timed) &&
      !cnd_init(&p_flags->cv)){
    p_flags->val = 0;
    retval = true;
  }
  return retval;
}

bool event_flag_free(event_flag_t *p_flags)
{
  return (p_flags && !pthread_cond_destroy(&p_flags->cv) && !pthread_mutex_destroy(&p_flags->mtx) );
}

static inline bool check_flags(event_flag_t *p_flags, uint32_t required_flags, event_flag_wait_type_e wait_type,
                               uint32_t *p_actual_flags)
{
  bool retval = false;
  uint32_t act_flags = p_flags->val & required_flags;
  if (wait_type == WAIT_OR || wait_type == WAIT_OR_CLEAR) {
    if (act_flags) {
      if (p_actual_flags) *p_actual_flags = act_flags;
      retval = true;
    }
  } else {
    if (act_flags == required_flags) {
      if (p_actual_flags) *p_actual_flags = act_flags;
      retval = true;
    }
  }
  if (retval && (wait_type == WAIT_AND_CLEAR || wait_type == WAIT_OR_CLEAR)) {
    p_flags->val &= ~act_flags;
  }
  return retval;
}

bool event_flag_wait(event_flag_t *p_flags, uint32_t required_flags, event_flag_wait_type_e wait_type,
                     uint32_t *p_actual_flags, uint32_t wait_ms)
{
  bool retval = false;
  struct timespec ts = {0};

  if (p_flags) {
    if (wait_ms != WAIT_FOREVER) {
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += wait_ms / 1000;
      ts.tv_nsec += (wait_ms % 1000) * 1000000;
    }
    int rval = 0;
    if (wait_ms == NO_SLEEP) {
      rval = pthread_mutex_trylock(&p_flags->mtx);
    } else {
      rval = pthread_mutex_timedlock(&p_flags->mtx, &ts);
    }
    CHECK_RETURNF(!rval, false, "Failed to acquire mutex, errno = %d", rval);
    while (!check_flags(p_flags, required_flags, wait_type, p_actual_flags)) {
      if (wait_ms != WAIT_FOREVER) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += wait_ms / 1000;
        ts.tv_nsec += (wait_ms % 1000) * 1000000;
        rval = pthread_cond_timedwait(&p_flags->cv, &p_flags->mtx, &ts);
      } else {
        rval = pthread_cond_wait(&p_flags->cv, &p_flags->mtx);
      }
      CHECK_RUN(!rval || (rval != ETIMEDOUT && rval != EINVAL && rval != EPERM),
                pthread_mutex_unlock(&p_flags->mtx);
                return retval;,
                "Condition Variable wait failed due to error: %d", rval);
      if (rval) {
        break;
      }
    }
    pthread_mutex_unlock(&p_flags->mtx);
    retval = true;
  }
  return retval;
}

bool event_flag_send(event_flag_t *p_flags, uint32_t flag_bits)
{
  bool retval = false;
  if(p_flags) {
    pthread_mutex_lock(&p_flags->mtx);
    p_flags->val |= flag_bits;
    pthread_cond_broadcast(&p_flags->cv);
    pthread_mutex_unlock(&p_flags->mtx);
    retval = true;
  }
  return retval;
}

bool event_flag_clear(event_flag_t *p_flags, uint32_t flag_bits)
{
  bool retval = false;
  if(p_flags) {
    pthread_mutex_lock(&p_flags->mtx);
    p_flags->val &= ~(flag_bits);
    pthread_mutex_unlock(&p_flags->mtx);
    retval = true;
  }
  return retval;
}
