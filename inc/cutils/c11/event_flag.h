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

typedef enum _event_flag_wait_type_e
{
  WAIT_OR,
  WAIT_OR_CLEAR,
  WAIT_AND,
  WAIT_AND_CLEAR
} event_flag_wait_type_e;

typedef struct _event_flag_t
{
  cnd_t cv;
  mtx_t mtx;
  uint32_t val;
} event_flag_t;

bool event_flag_new(event_flag_t *p_flags);

bool event_flag_free(event_flag_t *p_flags);

bool event_flag_wait(event_flag_t *p_flags, uint32_t required_flags, event_flag_wait_type_e wait_type,
                                   uint32_t *p_actual_flags, uint32_t wait_ms);

bool event_flag_send(event_flag_t *p_flags, uint32_t flag_bits);

bool event_flag_clear(event_flag_t *p_flags, uint32_t flag_bits);

#ifdef __cplusplus
};
#endif
