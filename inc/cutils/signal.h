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

#include <cutils/event_flag.h>


typedef event_flag_t signal_t;

static inline bool signal_new(signal_t *p_signal)
{
  return event_flag_new(p_signal);
}

static inline bool signal_free(signal_t *p_signal)
{
  return event_flag_free(p_signal);
}

static inline bool signal_wait(signal_t *p_signal)
{
  return event_flag_wait(p_signal, 1, WAIT_OR_CLEAR, NULL, WAIT_FOREVER);
}

static inline bool signal_wait_timed(signal_t *p_signal, uint32_t timeout)
{
  return event_flag_wait(p_signal, 1, WAIT_OR_CLEAR, NULL, timeout);
}
static inline bool signal_send(signal_t *p_signal)
{
  return event_flag_send(p_signal, 1);
}
