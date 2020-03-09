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

#pragma once

#include <cutils/types.h>
#include <cutils/logger.h>
#include <string.h>
#include <inttypes.h>

typedef struct
{
  uint8_t *buf;
  uint32_t total_buffer_size;
  uint32_t in, out;
} accumulator_t;

typedef struct
{
  accumulator_t *acc;
  uint8_t *buf;
  uint32_t buf_size;
} accumulator_create_params_t;

typedef void *accumulator_h;

#define ACCUMULATOR_STORE(name)   accumulator_store_##name
#define ACCUMULATOR_STORE_T(name) accumulator_store_##name##_t
#define ACCUMULATOR_STORE_DECL(name, size_in_bytes)\
typedef struct {\
  accumulator_t acc;\
  uint8_t buffer[size_in_bytes];\
}ACCUMULATOR_STORE_T(name)
#define ACCUMULATOR_STORE_DEF(name)\
ACCUMULATOR_STORE_T(name) ACCUMULATOR_STORE(name) __attribute__((section(".bss.noinit")))

#define ACCUMULATOR_CREATE_PARAMS_INIT(params, name)\
(params).acc = &ACCUMULATOR_STORE(name).acc;\
(params).buf = ACCUMULATOR_STORE(name).buffer;\
(params).buf_size = sizeof(ACCUMULATOR_STORE(name).buffer)


typedef uint32_t accumulator_iterator_t;

static inline accumulator_iterator_t accumulator_iterator_init(accumulator_h accumulator)
{
  return ((accumulator_t *) accumulator)->out;
}

static inline bool accumulator_iterator_valid(accumulator_h accumulator, accumulator_iterator_t iterator)
{
  accumulator_t *p_acc = (accumulator_t *) accumulator;
  return ((p_acc->in != p_acc->out) && iterator != p_acc->in);
}

static inline bool accumulator_iterator_next(accumulator_h accumulator, accumulator_iterator_t *iterator)
{
  accumulator_t *p_acc = (accumulator_t *) accumulator;
  if (accumulator_iterator_valid(accumulator, *iterator)) {
    *iterator = (*iterator + 1) % p_acc->total_buffer_size;
    return true;
  } else {
    //We've reached the end
    return false;
  }
}

static inline accumulator_h accumulator_create(accumulator_create_params_t *params)
{
  CUTILS_ASSERTF(params && params->acc, "Invalid params or accumulator static store is bad");
  accumulator_t *acc = params->acc;
  acc->buf = params->buf;
  acc->total_buffer_size = params->buf_size;
  acc->in = 0;
  acc->out = 0;
  return acc;
}

static inline void accumulator_clear(accumulator_h handle)
{
  accumulator_t *acc = (accumulator_t *) handle;
  acc->in = 0;
  acc->out = 0;
}

static inline uint32_t accumulator_bytes_left(accumulator_h handle)
{
  uint32_t retval = 0;
  accumulator_t *p_acc = (accumulator_t *) handle;

  if (p_acc->in >= p_acc->out) {
    retval = p_acc->total_buffer_size - (p_acc->in - p_acc->out);
  } else {
    retval = p_acc->out - p_acc->in;
  }
  return retval - 1;
}

static inline uint32_t accumulator_bytes_contained_from(accumulator_h handle, accumulator_iterator_t iter)
{
  uint32_t retval = 0;
  accumulator_t *p_acc = (accumulator_t *) handle;
  if (p_acc->in >= iter) {
    retval = p_acc->in - iter;
  } else {
    retval = p_acc->total_buffer_size - (iter - p_acc->in);
  }
  return retval;
}

static inline bool accumulator_iterator_advance(accumulator_h accumulator,
                                                accumulator_iterator_t *iterator,
                                                uint32_t size)
{
  bool retval = false;
  accumulator_t *p_acc = (accumulator_t *) accumulator;
  if (size <= accumulator_bytes_contained_from(accumulator, *iterator) &&
      ((*iterator + size) % p_acc->total_buffer_size) != p_acc->in) {
    *iterator = (*iterator + size) % p_acc->total_buffer_size;
    retval = true;
  }
  return retval;
}

static inline uint32_t accumulator_bytes_contained(accumulator_h handle)
{
  return accumulator_bytes_contained_from(handle, ((accumulator_t *) handle)->out);
}

static inline uint32_t accumulator_to_string(accumulator_h accumulator, char* str, uint32_t string_size)
{
  uint32_t retval = 0;
  accumulator_t *p_acc = (accumulator_t*)accumulator;
  if( string_size >=  128 ) {
    INSERT_TO_STRING(str, retval, "acc(in = %"PRIu32", out = %"PRIu32", contained = %"PRIu32", left = %"PRIu32", tot = %"PRIu32")",
                     p_acc->in,
                     p_acc->out,
                     accumulator_bytes_contained(accumulator),
                     accumulator_bytes_left(accumulator),
                     p_acc->total_buffer_size);
  }
  return retval;
}

static inline bool accumulator_insert(accumulator_h handle, uint8_t *buf, uint32_t size)
{
  bool retval = false;
  accumulator_t *p_acc = (accumulator_t *) handle;
  uint32_t bytes_left = accumulator_bytes_left(handle);

  if (size < p_acc->total_buffer_size) {
    if (size > bytes_left) {
      uint32_t amount_to_rotate = size - bytes_left;
      p_acc->out = (p_acc->out + amount_to_rotate) % p_acc->total_buffer_size;
      bytes_left = accumulator_bytes_left(handle);
    }
    if (size <= bytes_left) {
      if (p_acc->in + size > p_acc->total_buffer_size) {
        uint32_t remaining = size - (p_acc->total_buffer_size - p_acc->in);
        memcpy(&p_acc->buf[p_acc->in], buf, (p_acc->total_buffer_size - p_acc->in));
        memcpy(p_acc->buf, &buf[p_acc->total_buffer_size - p_acc->in], remaining);
      } else {
        memcpy(&p_acc->buf[p_acc->in], buf, size);
      }
      p_acc->in = (p_acc->in + size) % p_acc->total_buffer_size;
      retval = true;
    }
  }
  return retval;
}

static inline bool accumulator_peek(accumulator_h handle, uint8_t *buf, uint32_t size)
{
  bool retval = false;
  accumulator_t *p_acc = (accumulator_t *) handle;
  uint32_t bytes_contained = accumulator_bytes_contained(handle);
  if (size <= bytes_contained) {
    if (p_acc->out + size > p_acc->total_buffer_size) {
      uint32_t remaining = size - (p_acc->total_buffer_size - p_acc->out);
      memcpy(buf, &p_acc->buf[p_acc->out], (p_acc->total_buffer_size - p_acc->out));
      memcpy(&buf[p_acc->total_buffer_size - p_acc->out], p_acc->buf, remaining);
    } else {
      memcpy(buf, &p_acc->buf[p_acc->out], size);
    }
    retval = true;
  }
  return retval;
}

static inline bool accumulator_extract(accumulator_h handle, uint8_t *buf, uint32_t size)
{
  bool retval = false;
  accumulator_t *p_acc = (accumulator_t *) handle;
  if (accumulator_peek(handle, buf, size)) {
    retval = true;
    p_acc->out = (p_acc->out + size) % p_acc->total_buffer_size;
  }
  return retval;
}

static inline bool accumulator_advance(accumulator_h handle, uint32_t size)
{
  bool retval = false;
  accumulator_t *p_acc = (accumulator_t *) handle;
  if (size > p_acc->total_buffer_size - 1) {
    size = p_acc->total_buffer_size - 1;
  }
  p_acc->out = (p_acc->out + size) % p_acc->total_buffer_size;
  return retval;
}

static inline uint32_t
accumulator_peek_at(accumulator_h accumulator, accumulator_iterator_t iter, uint8_t *buf, uint32_t size)
{
  accumulator_t *p_acc = (accumulator_t *) accumulator;
  uint32_t bytes_contained = accumulator_bytes_contained_from(accumulator, iter);
  if (size > bytes_contained) {
    size = bytes_contained;
  }
  if (iter + size > p_acc->total_buffer_size) {
    uint32_t remaining = size - (p_acc->total_buffer_size - iter);
    memcpy(buf, &p_acc->buf[iter], (p_acc->total_buffer_size - iter));
    memcpy(&buf[p_acc->total_buffer_size - iter], p_acc->buf, remaining);
  } else {
    memcpy(buf, &p_acc->buf[iter], size);
  }
  return size;
}
