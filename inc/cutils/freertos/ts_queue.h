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

#include <FreeRTOS.h>
#include <cutils/os_types.h>
#include <queue.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ts_queue_t {
  QueueHandle_t handle;
  StaticQueue_t control_block;
} ts_queue_t;

#define TS_QUEUE_STORE(name)   _ts_queue_store_##name
#define TS_QUEUE_STORE_T(name) TS_QUEUE_STORE(name)##_t
#define TS_QUEUE_STORE_DECL(name, size)                                                            \
  static size_t _ts_queue_store_num_elements_##name = size;                                        \
  typedef struct {                                                                                 \
    uint8_t storage_array[size * sizeof(void *)];                                                  \
    ts_queue_t queue;                                                                              \
  } TS_QUEUE_STORE_T(name)

#define TS_QUEUE_STORE_DEF(name) static TS_QUEUE_STORE_T(name) TS_QUEUE_STORE(name)

typedef struct {
  ts_queue_t *queue;
  uint8_t *storage_array;
  size_t size;
} ts_queue_create_params_t;

#define TS_QUEUE_STORE_CREATE_PARAMS_INIT(params, name)                                            \
  memset(&(params), 0, sizeof((params)));                                                          \
  (params).queue = &TS_QUEUE_STORE(name).queue;                                                    \
  (params).storage_array = TS_QUEUE_STORE(name).storage_array;                                     \
  (params).size = _ts_queue_store_num_elements_##name

static inline ts_queue_t *ts_queue_init(ts_queue_create_params_t *params) {
  if (params) {
    ts_queue_t *retval = params->queue;
    retval->handle = xQueueCreateStatic(
        params->size, sizeof(void *), params->storage_array, &retval->control_block);
    return retval;
  }
  return NULL;
}

static inline void ts_queue_destroy(ts_queue_t *queue) {
  if (queue && queue->handle) {
    vQueueDelete(queue->handle);
    queue->handle = NULL;
  }
}

static inline bool ts_queue_enqueue(ts_queue_t *queue, void *item, uint32_t wait_ms) {
  if (queue && queue->handle) {
    TickType_t wait_ticks = (wait_ms == WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms);
    return xQueueSend(queue->handle, &item, wait_ticks) == pdPASS;
  }
  return false;
}

static inline bool ts_queue_dequeue(ts_queue_t *queue, void **item, uint32_t wait_ms) {
  if (queue && queue->handle) {
    TickType_t wait_ticks = (wait_ms == WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms);
    return xQueueReceive(queue->handle, item, wait_ticks) == pdPASS;
  }
  return false;
}

static inline size_t ts_queue_get_count(ts_queue_t *queue) {
  return (queue && queue->handle) ? uxQueueMessagesWaiting(queue->handle) : 0;
}

#ifdef __cplusplus
}
#endif
