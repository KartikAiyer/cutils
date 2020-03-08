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

#include <cutils/task.h>
#include <cutils/logger.h>
#include <pthread.h>
#include <string.h>

#define TASK_SANITY (0xDEADBEEF)
static pthread_key_t s_task_private_key = 0;
static pthread_once_t s_init = PTHREAD_ONCE_INIT;

static void thread_init(void)
{
  pthread_key_create(&s_task_private_key, 0);
}

static void* task_runner(void* ctx)
{
  task_t *task = (task_t*)ctx;
  pthread_once(&s_init, thread_init);
  pthread_setspecific(s_task_private_key, task);
  task->func(task->ctx);
  pthread_setspecific(s_task_private_key, NULL);
}

task_t *task_new_static(task_create_params_t *create_params)
{
  task_t *retval = 0;

  if (create_params &&
      create_params->task &&
      create_params->func  && create_params->stack && create_params->stack_size) {

    pthread_attr_t attr = {0};
    size_t min_stack_size = 0;

    memset(create_params->task, 0, sizeof(task_t));
    pthread_attr_init(&attr);

    int rval = pthread_attr_getstacksize(&attr, &min_stack_size);
    CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to query minimum stack size");
    rval = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to set scheduling parameter");
    rval = pthread_attr_setschedpolicy(&attr, SCHEDULING_POLICY);
    CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to set scheduling policy");
    struct sched_param param = {.sched_priority = create_params->priority};
    rval = pthread_attr_setschedparam(&attr, &param);
    CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to set priority to %d", param.sched_priority);
    if(create_params->stack && create_params->stack_size >= min_stack_size) {
      rval = pthread_attr_setstack(&attr, create_params->stack, create_params->stack_size);
      CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to set stack", create_params->stack);
    }
    CHECK_RUN(!rval, pthread_attr_destroy(&attr); return retval, "Failed to set scheduling priority between (%d, %d)",
              sched_get_priority_min(SCHEDULING_POLICY), sched_get_priority_max(SCHEDULING_POLICY) );
    create_params->task->ctx = create_params->ctx;
    create_params->task->func = create_params->func;
    int res = pthread_create(&create_params->task->task, &attr, task_runner, create_params->task);
    create_params->task->sanity = TASK_SANITY;
    strncpy(create_params->task->label, create_params->label, sizeof(create_params->task->label) -1);
    //TODO: Add an assertion
    retval = (res == 0) ? create_params->task : 0;
  }
  return retval;
}

void task_destroy_static(task_t *task)
{
  if(task) {
    int res = 0;
    pthread_join(task->task, (void**)&res);
  }
}
bool task_start(task_t *task)
{
  return true;
}

uint64_t task_get_ticks(void)
{
  struct timespec res = {0};
  clock_gettime(CLOCK_REALTIME, &res);
  return res.tv_sec * 1000000000 + res.tv_nsec;
}

void task_sleep(uint32_t ms)
{
  struct timespec duration = {.tv_sec = ms/1000, .tv_nsec = ( ms % 1000 ) * 1000000};
  nanosleep(&duration, 0);
}

void task_get_current_name(char* name, size_t string_len)
{
  task_t* current = pthread_getspecific(s_task_private_key);
  if(current && current->sanity == TASK_SANITY) {
    strncpy(name, current->label, string_len);
  } else {
    strncpy(name, "unknown", string_len);
  }
}

