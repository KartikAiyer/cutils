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
#include <event_groups.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The representation of an event flag object on the FreeRTOS port.
 * 
 * @warning FreeRTOS Event Groups have a limitation on the number of bits available.
 *          Typically, only 24 bits are usable as the upper 8 bits are reserved for
 *          internal tracking by the kernel.
 */
typedef struct _event_flag_t {
  StaticEventGroup_t control_block;
  EventGroupHandle_t handle;
} event_flag_t;

/** 
 * @brief Mask used to ensure only usable FreeRTOS event bits are passed to API calls.
 *        Prevents triggering configASSERT when passing full 32-bit masks.
 */
#define EVENT_FLAG_VALID_BITS 0x00FFFFFFU

static inline bool event_flag_new(event_flag_t *flags) {
  if (flags) {
    flags->handle = xEventGroupCreateStatic(&flags->control_block);
    return flags->handle != NULL;
  }
  return false;
}

static inline bool event_flag_free(event_flag_t *flags) {
  if (flags && flags->handle) {
    vEventGroupDelete(flags->handle);
    flags->handle = NULL;
    return true;
  }
  return false;
}

static inline bool event_flag_wait(event_flag_t *flags,
                                   uint32_t required_flags,
                                   event_flag_wait_type_e wait_type,
                                   uint32_t *actual_flags,
                                   uint32_t wait_ms) {
  if (flags && flags->handle) {
    TickType_t wait_ticks = (wait_ms == WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms);
    BaseType_t xWaitForAllBits = pdFALSE;
    BaseType_t xClearOnExit = pdFALSE;

    switch (wait_type) {
    case WAIT_OR:
      xClearOnExit = pdFALSE;
      xWaitForAllBits = pdFALSE;
      break;
    case WAIT_OR_CLEAR:
      xClearOnExit = pdTRUE;
      xWaitForAllBits = pdFALSE;
      break;
    case WAIT_AND:
      xClearOnExit = pdFALSE;
      xWaitForAllBits = pdTRUE;
      break;
    case WAIT_AND_CLEAR:
      xClearOnExit = pdTRUE;
      xWaitForAllBits = pdTRUE;
      break;
    default:
      break;
    }
    EventBits_t got = xEventGroupWaitBits(
        flags->handle, required_flags & EVENT_FLAG_VALID_BITS, xClearOnExit, xWaitForAllBits, wait_ticks);
    if (actual_flags)
      *actual_flags = (uint32_t)got;
    if (xWaitForAllBits == pdTRUE) {
      return ((got & required_flags) == required_flags);
    } else {
      return ((got & required_flags) != 0);
    }
  }
  return false;
}

static inline bool event_flag_send(event_flag_t *flags, uint32_t flag_bits) {
  if (flags && flags->handle) {
    xEventGroupSetBits(flags->handle, (EventBits_t)(flag_bits & EVENT_FLAG_VALID_BITS));
    return true;
  }
  return false;
}

static inline bool event_flag_clear(event_flag_t *flags, uint32_t flag_bits) {
  if (flags && flags->handle) {
    xEventGroupClearBits(flags->handle, (EventBits_t)(flag_bits & EVENT_FLAG_VALID_BITS));
    return true;
  }
  return false;
}

#ifdef __cplusplus
}
#endif
