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
 #pragma once


#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#endif

#include <cutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief These macros are used in api's that block and have a parameter to specify the block duration.
 */
#define WAIT_FOREVER          ((uint32_t)-1)
#define NO_SLEEP              (0)

/**
 * @brief Prototype for callbacks used by dispatch apis. Allows to pass two context arguments whose durations should
 * be alive for the duration of the dispatch API being used.
 */
typedef void (*dispatch_function_t)(void *arg1, void *arg2);

/**
 * @brief Aligned predicate to be used by dispatch once
 */
typedef atomic_uint dispatch_predicate_t;

typedef void *dispatch_queue_timed_action_h;

typedef struct _dispatch_queue_t dispatch_queue_t;

#ifdef __cplusplus
};
#endif
