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
#include <semphr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mutex_t {
  StaticSemaphore_t cb;
  SemaphoreHandle_t handle;
} mutex_t;

static inline bool mutex_new(mutex_t *mutex) {
  if (mutex) {
    mutex->handle = xSemaphoreCreateMutexStatic(&mutex->cb);
    return mutex->handle != NULL;
  }
  return false;
}

static inline void mutex_free(mutex_t *mutex) {
  if (mutex && mutex->handle) {
    vSemaphoreDelete(mutex->handle);
    mutex->handle = NULL;
  }
}

static inline bool mutex_lock(mutex_t *mutex, uint32_t wait_ms) {
  if (mutex && mutex->handle) {
    TickType_t wait_ticks = (wait_ms == WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms);
    return xSemaphoreTake(mutex->handle, wait_ticks) == pdTRUE;
  }
  return false;
}

static inline bool mutex_unlock(mutex_t *mutex) {
  if (mutex && mutex->handle) {
    return xSemaphoreGive(mutex->handle) == pdTRUE;
  }
  return false;
}

#ifdef __cplusplus
}
#endif
