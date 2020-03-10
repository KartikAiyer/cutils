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

#include <cutils/log_buffer.h>
#include <cutils/mutex.h>
#include <stdarg.h>
#include <string.h>


/**
 * @file ts_log_buffer.h
 * This file provides a thread safe API around log_buffers. It also allows clients to register notifications for
 * log buffer size events.
 */

#define LOG_BUF_NOTIF(XX)\
  XX( LB_FULL, )\
  XX( LB_ALMOST_FULL, )\
  XX( LB_EMPTY, )\

DECLARE_ENUM(ts_log_buffer_notfication_e, LOG_BUF_NOTIF)

struct _ts_log_buffer_t;

/**
 * @brief this callback signature allows clients to be notified when the log buffer hits certain
 * size thresholds.
 */
typedef void (*ts_log_buffer_f)(struct _ts_log_buffer_t *pLb, ts_log_buffer_notfication_e notif,
                                void *private);

typedef struct _ts_log_buffer_t
{
  log_buffer_t lb;
  mutex_t mtx;
  ts_log_buffer_f fn;
  void *private;
  char line_buffer[2 * LOG_MAX_LINE_LENGTH];
} ts_log_buffer_t;

/**
 * @brief Initializes the thread safe wrapper to a log_buffer
 * @param pLb - Pointer to thread safe log buffer control block. This control block will
 *              be initialized for use after this call
 * @param pBuffer - Backing buffer for the log buffer
 * @param bufferSize - Backing buffer size.
 * @return - true if successfully initialized.
 */
bool ts_log_buffer_init(ts_log_buffer_t *pLb, char *pBuffer, uint32_t bufferSize);

/**
 * @brief Deinitializes the thread safe wrapper to the log_buffer
 * @param pLb - Initialized thread safe log buffer
 */
void ts_log_buffer_deinit(ts_log_buffer_t *pLb);

/**
 * @brief Thread safe wrapper to log_buffer_push
 * @param p_ts_log - Initialized log buffer
 * @param stringa- string to insert
 * @param string_size - size of string
 */
void ts_log_buffer_push(ts_log_buffer_t *p_ts_log, char *string, uint32_t string_size);

/**
 * @brief Allows clients to install notifications for log buffer size events.
 * @param p_ts_log - Initialized log buffer
 * @param fn - callback
 * @param private - callback context
 */
void ts_log_buffer_install_notifications(ts_log_buffer_t *p_ts_log, ts_log_buffer_f fn, void *private);

/**
 * @brief Returns the current size of the log_buffer.
 * @param p_lb - Initialized Log Buffer
 * @param initial_bytes - If there is no wrap around, this is the total byte size
 * @param residual_bytes - If a wrap around occurs, this is the number of bytes from the head of the buffer to the last
 *                          character inserted
 * @return
 */
uint32_t ts_log_buffer_current_size(ts_log_buffer_t *p_lb, uint32_t *initial_bytes, uint32_t *residual_bytes);
