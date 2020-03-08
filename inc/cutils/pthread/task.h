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

#include <cutils/os_types.h>
#include <pthread.h>

#define SCHEDULING_POLICY               (SCHED_OTHER)
#ifdef __cplusplus
extern "C" {
#endif

#define CUTILS_TASK_PRIORITY_LOWEST     (sched_get_priority_min(SCHEDULING_POLICY))
#define CUTILS_TASK_PRIORITY_HIGHEST    (sched_get_priority_max(SCHEDULING_POLICY))
#define CUTILS_TASK_PRIORITY_MEDIUM     (( CUTILS_TASK_PRIORITY_LOWEST + CUTILS_TASK_PRIORITY_HIGHEST ) / 2)
#define CUTILS_TASK_PRIORITY_MID_HIGH   (CUTILS_TASK_PRIORITY_MEDIUM - ((CUTILS_TASK_PRIORITY_HIGHEST - CUTILS_TASK_PRIORITY_LOWEST)/4))
#define CUTILS_TASK_PRIORITY_MID_LO     (CUTILS_TASK_PRIORITY_MEDIUM + ((CUTILS_TASK_PRIORITY_HIGHEST - CUTILS_TASK_PRIORITY_LOWEST)/4))

#define CUTILS_TASK_STACK_MIN_SIZE               (PTHREAD_STACK_MIN)
#define DEFAULT_TASK_PRIORITY CUTILS_TASK_PRIORITY_MEDIUM

typedef void (*task_func_t)(void*);

typedef struct _task_t
{
  pthread_t task;
  uint32_t sanity;
  char label[30];
  task_func_t func;
  void *ctx;
} task_t;

typedef struct _task_create_params_t
{
  task_t *task;
  char *label;
  uint32_t priority;
  task_func_t func;
  void *ctx;
  void *stack;
  uint32_t stack_size;
} task_create_params_t;

#define TASK_STATIC_STORE_T(name) task_static_store_##name##_t

#define TASK_STATIC_STORE(name) _task_store_##name

#define TASK_STATIC_STORE_DECL(name, stack_size)\
typedef struct {\
  uint8_t stack[ (stack_size) ];\
  task_t tsk;\
}TASK_STATIC_STORE_T( name )

#define TASK_STATIC_STORE_DEF(name)\
TASK_STATIC_STORE_T( name ) TASK_STATIC_STORE(name) __attribute__((section(".bss.noinit"),used))

#define TASK_STATIC_INIT_CREATE_PARAMS(params, name, lbl, pri, fn, context)\
  memset(&(params), 0, sizeof((params)));\
  (params).task = &TASK_STATIC_STORE(name).tsk;\
  (params).label = (char*)(lbl);\
  (params).priority = pri;\
  (params).func = fn;\
  (params).ctx = context;\
  (params).stack = TASK_STATIC_STORE(name).stack;\
  (params).stack_size = sizeof(TASK_STATIC_STORE(name).stack)

task_t *task_new_static(task_create_params_t *create_params);

bool task_start(task_t *task);

void task_destroy_static(task_t *task);

uint64_t task_get_ticks(void);

void task_sleep(uint32_t ms);

void task_get_current_name(char *name, size_t string_length);

#ifdef __cplusplus
}
#endif