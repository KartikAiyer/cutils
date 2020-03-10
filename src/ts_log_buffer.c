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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DACUTILSS OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cutils/ts_log_buffer.h>
#include <stdio.h>

bool ts_log_buffer_init(ts_log_buffer_t *pLb, char *pBuffer, uint32_t bufferSize)
{
  bool retval = false;

  if (pLb) {
    if (mutex_new(&pLb->mtx)) {
      retval = log_buffer_init(&pLb->lb, pBuffer, bufferSize);
    }
  }
  return retval;
}

void ts_log_buffer_deinit(ts_log_buffer_t *pLb)
{
  if (pLb) {
    mutex_free(&pLb->mtx);
  }
}

void ts_log_buffer_push(ts_log_buffer_t *p_ts_log, char *string, uint32_t string_size)
{
  if (p_ts_log) {
    mutex_lock(&p_ts_log->mtx, WAIT_FOREVER);
    if (p_ts_log->lb.isInit) {
      log_buffer_push(&p_ts_log->lb, string, string_size);
      ts_log_buffer_f fn = p_ts_log->fn;
      void *private = p_ts_log->private;
      uint32_t used_lines = log_buffer_lines_from_size(log_buffer_current_size(&p_ts_log->lb, NULL, NULL));
      const uint32_t max_lines = log_buffer_lines_from_size(p_ts_log->lb.bufferSize) - 1;
      int32_t diff = max_lines - used_lines;

      mutex_unlock(&p_ts_log->mtx);
      if (diff > 0 && diff < 3 && fn) {
        // Notifiy client that space is running out.
        fn(p_ts_log, LB_ALMOST_FULL, private);
      } else if (diff <= 0 && fn) {
        fn(p_ts_log, LB_FULL, private);
      }
    } else {
      mutex_unlock(&p_ts_log->mtx);
    }
  }
}

void ts_log_buffer_install_notifications(ts_log_buffer_t *p_ts_log, ts_log_buffer_f fn, void *private)
{
  if (p_ts_log) {
    mutex_lock(&p_ts_log->mtx, WAIT_FOREVER);
    p_ts_log->fn = fn;
    p_ts_log->private = private;
    mutex_unlock(&p_ts_log->mtx);
  }
}

uint32_t ts_log_buffer_current_size(ts_log_buffer_t *p_lb, uint32_t *initial_bytes, uint32_t *residual_bytes)
{
  uint32_t retval = 0;

  if (p_lb) {
    mutex_lock(&p_lb->mtx, WAIT_FOREVER);
    retval = log_buffer_current_size(&p_lb->lb, initial_bytes, residual_bytes);
    mutex_unlock(&p_lb->mtx);
  }
  return retval;
}
