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
#include <stdalign.h>
#include <task.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name Task Priority Levels
 *  These macros define the priority range for tasks on this port.
 *  @note In FreeRTOS, higher numbers typically represent higher priorities.
 *  @{ */
#define CUTILS_TASK_PRIORITY_LOWEST (0)
#define CUTILS_TASK_PRIORITY_HIGHEST                                                               \
  (configMAX_PRIORITIES - 2) // Timer thread takes configMAX_PRIORITIES - 1
#define CUTILS_TASK_PRIORITY_MEDIUM                                                                \
  ((CUTILS_TASK_PRIORITY_LOWEST + CUTILS_TASK_PRIORITY_HIGHEST) / 2)
#define CUTILS_TASK_PRIORITY_MID_LO                                                                \
  (CUTILS_TASK_PRIORITY_MEDIUM - ((CUTILS_TASK_PRIORITY_HIGHEST - CUTILS_TASK_PRIORITY_LOWEST) / 4))
#define CUTILS_TASK_PRIORITY_MID_HIGH                                                              \
  (CUTILS_TASK_PRIORITY_MEDIUM + ((CUTILS_TASK_PRIORITY_HIGHEST - CUTILS_TASK_PRIORITY_LOWEST) / 4))
/** @} */

#define CUTILS_TASK_STACK_MIN_SIZE (configMINIMAL_STACK_SIZE)
#define DEFAULT_TASK_PRIORITY      (CUTILS_TASK_PRIORITY_MEDIUM)

// You can bring your own stack alignment value, otherwise we will use the FreeRTOS default
#ifndef CUTILS_TASK_STACK_ALIGN
#define CUTILS_TASK_STACK_ALIGN portBYTE_ALIGNMENT
#endif

/**
 * @brief Function signature for task entry points.
 */
typedef void (*task_func_t)(void *);

/**
 * @brief The internal representation of a task on the FreeRTOS port.
 */
typedef struct _task_t {
  TaskHandle_t task;
  StaticTask_t tcb;
  uint32_t sanity;
  task_func_t func;
  void *ctx;
} task_t;

/**
 * @brief Parameters used to initialize a task.
 */
typedef struct _task_create_params_t {
  task_t *task;
  StaticTask_t *tcb;
  char *label;
  size_t priority;
  task_func_t func;
  void *ctx;
  void *stack;
  size_t stack_size;
} task_create_params_t;

/**
 * @name Static Task Storage Macros
 *  These macros provide a mechanism for statically allocating tasks and their stacks.
 *  @{ */
#define TASK_STATIC_STORE_T(name) task_static_store_##name##_t

/** @brief The symbol name for the static store of a given task name. */
#define TASK_STATIC_STORE(name) _task_store_##name

/**
 * @brief Declares a structure that holds both the stack and the task control block.
 */
#define TASK_STATIC_STORE_DECL(name, stack_size)                                                   \
  typedef struct {                                                                                 \
    alignas(CUTILS_TASK_STACK_ALIGN) uint8_t stack[(stack_size)];                                  \
    task_t tsk;                                                                                    \
  } TASK_STATIC_STORE_T(name)

/** @brief Defines the static store as a used symbol. */
#define TASK_STATIC_STORE_DEF(name)                                                                \
  TASK_STATIC_STORE_T(name) TASK_STATIC_STORE(name) __attribute__((used))

/** @brief Internal helper to map a static store to task create parameters. */
#define TASK_INIT_CREATE_PARAMS_FROM_STORE(params, store_ptr, lbl, pri, fn, context)               \
  memset(&(params), 0, sizeof((params)));                                                          \
  (params).task = &(store_ptr)->tsk;                                                               \
  (params).tcb = &((store_ptr)->tsk.tcb);                                                          \
  (params).label = (char *)(lbl);                                                                  \
  (params).priority = pri;                                                                         \
  (params).func = fn;                                                                              \
  (params).ctx = context;                                                                          \
  (params).stack = (store_ptr)->stack;                                                             \
  (params).stack_size = sizeof((store_ptr)->stack)

/** @brief Public macro to initialize create parameters from a named static store. */
#define TASK_STATIC_INIT_CREATE_PARAMS(params, name, lbl, pri, fn, context)                        \
  TASK_INIT_CREATE_PARAMS_FROM_STORE(params, &TASK_STATIC_STORE(name), lbl, pri, fn, context)
/** @} */

#ifdef __cplusplus
}
#endif
