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
#define TS_QUEUE_STORE_T(name) _ts_queue_store_##name##_t
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
    if (params->size == 0 || (params->size & (params->size - 1)) != 0)
      return NULL;
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

/**
 * @brief Attempts to enqueue an item on the queue from an ISR.
 *
 * ISR-safe counterpart to ts_queue_enqueue(), backed by @c xQueueSendFromISR(). There is no
 * @c wait_ms parameter by design: the absence of a timeout @e is the guarantee that this call
 * never blocks. If the queue is full the call returns @c false immediately, leaving the calling
 * ISR to handle the error as it sees fit.
 *
 * The act of queueing can unblock a task that was waiting to receive from this queue. If that
 * task is a higher priority than the one the ISR interrupted, the ISR should request a context
 * switch before exiting so the higher priority task runs promptly rather than at the next tick.
 * That condition is reported through @p higher_priority_task_woken.
 *
 * @param queue Queue to enqueue into. A NULL queue, or one that was never successfully
 *              initialized, fails the call rather than faulting.
 * @param item Item to enqueue. The queue stores the pointer value itself, not the pointed-to
 *             data, so the referent must remain valid until the consumer is done with it.
 * @param[out] higher_priority_task_woken Set to @c true only if a higher priority task was
 *             actually woken, and left untouched otherwise — this function never reads it and
 *             never clears it. A sequence of such calls across different queues can therefore
 *             share one flag and yield once at the end: a later call that woke nobody will not
 *             overwrite an earlier call's @c true. May be NULL, which means the caller accepts
 *             a deferred context switch.
 *
 * @return @c true if the item was enqueued, @c false if the queue was full or @p queue was
 *         invalid.
 *
 * @warning Only call this from an ISR whose priority is numerically greater than or equal to
 *          @c configMAX_SYSCALL_INTERRUPT_PRIORITY. Calling it from a higher-urgency (lower
 *          numbered) interrupt corrupts kernel state. Use ts_queue_enqueue() from task context.
 *
 * @note The caller owns the single yield, and must initialize the flag itself:
 * @code
 * void my_isr(void) {
 *   bool woken = false;
 *   ts_queue_enqueue_from_isr(q1, a, &woken);
 *   ts_queue_enqueue_from_isr(q2, b, &woken);
 *   if (woken) portYIELD_FROM_ISR(pdTRUE);
 * }
 * @endcode
 *
 * @see ts_queue_dequeue_from_isr()
 */
static inline bool
ts_queue_enqueue_from_isr(ts_queue_t *queue, void *item, bool *higher_priority_task_woken) {
  if (queue && queue->handle) {
    BaseType_t woken = pdFALSE;
    BaseType_t res = xQueueSendFromISR(queue->handle, &item, &woken);
    if (higher_priority_task_woken && woken)
      *higher_priority_task_woken = true;
    return (res == pdPASS);
  }
  return false;
}

/**
 * @brief Attempts to dequeue an item from the queue from an ISR.
 *
 * ISR-safe counterpart to ts_queue_dequeue(), backed by @c xQueueReceiveFromISR(). There is no
 * @c wait_ms parameter by design: the absence of a timeout @e is the guarantee that this call
 * never blocks. If the queue is empty the call returns @c false immediately, leaving the calling
 * ISR to handle the error as it sees fit.
 *
 * The act of dequeueing frees a slot, which can unblock a task that was waiting to send into a
 * full queue. If that task is a higher priority than the one the ISR interrupted, the ISR should
 * request a context switch before exiting so the higher priority task runs promptly rather than
 * at the next tick. That condition is reported through @p higher_priority_task_woken. Note this
 * is the mirror of the enqueue case: here the task being woken is a blocked @e producer, so the
 * flag can only ever be set when the queue was full on entry.
 *
 * @param queue Queue to dequeue from. A NULL queue, or one that was never successfully
 *              initialized, fails the call rather than faulting.
 * @param[out] item Receives the dequeued pointer. Only written when the call returns @c true.
 *             Must not be NULL.
 * @param[out] higher_priority_task_woken Set to @c true only if a higher priority task was
 *             actually woken, and left untouched otherwise — this function never reads it and
 *             never clears it. A sequence of such calls across different queues can therefore
 *             share one flag and yield once at the end: a later call that woke nobody will not
 *             overwrite an earlier call's @c true. May be NULL, which means the caller accepts
 *             a deferred context switch.
 *
 * @return @c true if an item was dequeued, @c false if the queue was empty or @p queue was
 *         invalid.
 *
 * @warning Only call this from an ISR whose priority is numerically greater than or equal to
 *          @c configMAX_SYSCALL_INTERRUPT_PRIORITY. Calling it from a higher-urgency (lower
 *          numbered) interrupt corrupts kernel state. Use ts_queue_dequeue() from task context.
 *
 * @note The caller owns the single yield, and must initialize the flag itself:
 * @code
 * void my_isr(void) {
 *   void *item = NULL;
 *   bool woken = false;
 *   while (ts_queue_dequeue_from_isr(q, &item, &woken))
 *     handle(item);
 *   if (woken) portYIELD_FROM_ISR(pdTRUE);
 * }
 * @endcode
 *
 * @see ts_queue_enqueue_from_isr()
 */
static inline bool
ts_queue_dequeue_from_isr(ts_queue_t *queue, void **item, bool *higher_priority_task_woken) {
  if (queue && queue->handle) {
    BaseType_t woken = pdFALSE;
    BaseType_t res = xQueueReceiveFromISR(queue->handle, item, &woken);
    if (higher_priority_task_woken && woken)
      *higher_priority_task_woken = true;
    return (res == pdPASS);
  }
  return false;
}

static inline size_t ts_queue_get_count(ts_queue_t *queue) {
  return (queue && queue->handle) ? uxQueueMessagesWaiting(queue->handle) : 0;
}

#ifdef __cplusplus
}
#endif
