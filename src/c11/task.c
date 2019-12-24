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

#include <cutils/task.h>
#include <cutils/logger.h>
#include <string.h>

task_t *task_new_static(task_create_params_t *create_params)
{
  task_t *retval = 0;

  if (create_params &&
      create_params->task &&
      create_params->func  && create_params->stack && create_params->stack_size) {
    memset(create_params->task, 0, sizeof(task_t));
    int r = thrd_create_ex(&create_params->task->task,
                            create_params->func,
                            create_params->ctx,
                            create_params->label,
                            create_params->priority,
                            create_params->stack,
                            create_params->stack_size);

    strncpy(create_params->task->label, create_params->label, sizeof(create_params->task->label) -1);
    //TODO: Add an assertion
    retval = (r == 0) ? create_params->task : 0;
  }
  return retval;
}

void task_destroy_static(task_t *task)
{
  if(task) {
    thrd_exit(0);
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
  thrd_sleep(&duration, 0);
}

void task_get_current_name(char* name, size_t string_len)
{
  pthread_getname_np(thrd_current(), name, string_len);
}
