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

#ifndef CUTILS_C11THREADS_H
#define CUTILS_C11THREADS_H
#ifndef C11THREADS_H_
#define C11THREADS_H_

#include <cutils/logger.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h> /* for sched_yield */
#include <sys/time.h>
#include <time.h>

#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT

/* Scheduling policy for the c11 port (which calls into pthread underneath).
 * Configurable at CMake time via CUTILS_PTHREAD_SCHED_POLICY (default
 * SCHED_OTHER). Same priority-abstraction contract as the pthread port: under
 * SCHED_OTHER the CUTILS_TASK_PRIORITY_* range is 0..0, .priority is a no-op
 * and thrd_create_ex() inherits the parent's scheduling (host/CI, runs
 * unprivileged); under SCHED_FIFO/SCHED_RR priorities are real (1..99) via
 * PTHREAD_EXPLICIT_SCHED, which requires CAP_SYS_NICE on Linux — intended for
 * the embedded-Linux production target. See issue #22. */
#ifndef CUTILS_PTHREAD_SCHED_POLICY
#define CUTILS_PTHREAD_SCHED_POLICY SCHED_OTHER
#endif
#define SCHEDULING_POLICY (CUTILS_PTHREAD_SCHED_POLICY)

#ifdef __APPLE__
/* Darwin doesn't implement timed mutexes currently */
#define C11THREADS_NO_TIMED_MUTEX
#endif

#ifdef C11THREADS_NO_TIMED_MUTEX
#define PTHREAD_MUTEX_TIMED_NP PTHREAD_MUTEX_NORMAL
#define C11THREADS_TIMEDLOCK_POLL_INTERVAL 5000000 /* 5 ms */
#endif

/* types */
typedef pthread_t thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t cnd_t;
typedef pthread_key_t tss_t;

typedef int (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

enum {
  mtx_plain = 0,
  mtx_recursive = 1,
  mtx_timed = 2,
};

enum { thrd_success, thrd_timedout, thrd_busy, thrd_error, thrd_nomem };

/* ---- thread management ---- */

/* Internal bridge from our int-returning thrd_start_t to the void*-returning
 * start routine pthread_create expects. Casting between incompatible function
 * pointer types is technically UB and trips -Wcast-function-type; thrd_join
 * reinterprets the return value back to int anyway, so the cast is benign on
 * every ABI we target. Isolate it here with the warning silenced locally so
 * neither thrd_create nor thrd_create_ex exposes the cast at its call site.
 * See issue #23. */
static inline int c11threads_pthread_create(thrd_t *thr,
                                            const pthread_attr_t *attr,
                                            thrd_start_t func,
                                            void *arg) {
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
  void *(*start)(void *) = (void *(*)(void *))func;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
  return pthread_create(thr, attr, start, arg);
}

static inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
  pthread_attr_t attr = {0};
  pthread_attr_init(&attr);

  int res = c11threads_pthread_create(thr, &attr, func, arg);
  if (res == 0) {
    return thrd_success;
  }
  pthread_attr_destroy(&attr);
  return res == ENOMEM ? thrd_nomem : thrd_error;
}

static inline int thrd_create_ex(thrd_t *thr,
                                 thrd_start_t func,
                                 void *arg,
                                 char *label,
                                 int priority,
                                 void *stack,
                                 size_t stack_size) {
  pthread_attr_t attr = {0};
  size_t min_stack_size = 0;
  pthread_attr_init(&attr);
  /* label is stored by the task layer (see task_get_current_name in
   * src/c11/task.c, which reads a thread-local task pointer); thread naming
   * via pthread_setname_np was removed for portability since it is a glibc
   * Linux-only (_GNU_SOURCE) extension. See issue #23. */
  (void)label;

  int rval = pthread_attr_getstacksize(&attr, &min_stack_size);
  CHECK_RUN(!rval, pthread_attr_destroy(&attr); return rval, "Failed to query minimum stack size");
  /* Only real-time policies expose a non-trivial priority range; gate
   * PTHREAD_EXPLICIT_SCHED on SCHED_FIFO/SCHED_RR (needs CAP_SYS_NICE on
   * Linux). Under the default SCHED_OTHER, .priority is a no-op and we inherit
   * the parent's scheduling so the build runs unprivileged. See issue #22. */
#if CUTILS_PTHREAD_SCHED_POLICY == SCHED_FIFO || \
    CUTILS_PTHREAD_SCHED_POLICY == SCHED_RR
  rval = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  CHECK_RUN(!rval, pthread_attr_destroy(&attr); return rval, "Failed to set scheduling parameter");
  rval = pthread_attr_setschedpolicy(&attr, SCHEDULING_POLICY);
  CHECK_RUN(!rval, pthread_attr_destroy(&attr); return rval, "Failed to set scheduling policy");
  struct sched_param param = {.sched_priority = priority};
  rval = pthread_attr_setschedparam(&attr, &param);
  CHECK_RUN(!rval, pthread_attr_destroy(&attr);
            return rval, "Failed to set priority to %d", param.sched_priority);
#else
  /* SCHED_OTHER: priority range is 0..0, so .priority is a no-op. Inherit the
   * creating thread's scheduling to avoid the CAP_SYS_NICE requirement. */
  (void)priority;
  rval = pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
  CHECK_RUN(!rval, pthread_attr_destroy(&attr); return rval, "Failed to set inherit scheduling");
#endif
  if (stack && stack_size >= min_stack_size) {
    rval = pthread_attr_setstack(&attr, stack, stack_size);
    CHECK_RUN(!rval, pthread_attr_destroy(&attr);
              return rval, "Failed to set stack", stack);
  }
  CHECK_RUN(!rval, pthread_attr_destroy(&attr);
            return rval,
                   "Failed to set scheduling priority between (%d, %d)",
                   sched_get_priority_min(SCHEDULING_POLICY),
                   sched_get_priority_max(SCHEDULING_POLICY));
  int res = c11threads_pthread_create(thr, &attr, func, arg);
  pthread_attr_destroy(&attr);
  if (res == 0) {
    return thrd_success;
  } else {
    return thrd_error;
  }
}

static inline void thrd_exit(int res) { pthread_exit((void *)&res); }

static inline int thrd_join(thrd_t thr, int *res) {
  void *retval;

  if (pthread_join(thr, &retval) != 0) {
    return thrd_error;
  }
  if (res) {
    *res = *((int *)&retval);
  }
  return thrd_success;
}

static inline int thrd_detach(thrd_t thr) {
  return pthread_detach(thr) == 0 ? thrd_success : thrd_error;
}

static inline thrd_t thrd_current(void) { return pthread_self(); }

static inline int thrd_equal(thrd_t a, thrd_t b) { return pthread_equal(a, b); }

static inline int thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out) {
  if (nanosleep(ts_in, rem_out) < 0) {
    if (errno == EINTR)
      return -1;
    return -2;
  }
  return 0;
}

static inline void thrd_yield(void) { sched_yield(); }

/* ---- mutexes ---- */

static inline int mtx_init(mtx_t *mtx, int type) {
  int res;
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);

  if (type & mtx_timed) {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
  }
  if (type & mtx_recursive) {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  }

  res = pthread_mutex_init(mtx, &attr) == 0 ? thrd_success : thrd_error;
  pthread_mutexattr_destroy(&attr);
  return res;
}

static inline void mtx_destroy(mtx_t *mtx) { pthread_mutex_destroy(mtx); }

static inline int mtx_lock(mtx_t *mtx) {
  int res = pthread_mutex_lock(mtx);
  if (res == EDEADLK) {
    return thrd_busy;
  }
  return res == 0 ? thrd_success : thrd_error;
}

static inline int mtx_trylock(mtx_t *mtx) {
  int res = pthread_mutex_trylock(mtx);
  if (res == EBUSY) {
    return thrd_busy;
  }
  return res == 0 ? thrd_success : thrd_error;
}

static inline int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
  int res;
#ifdef C11THREADS_NO_TIMED_MUTEX
  /* fake a timedlock by polling trylock in a loop and waiting for a bit */
  struct timeval now;
  struct timespec sleeptime;

  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = C11THREADS_TIMEDLOCK_POLL_INTERVAL;

  while ((res = pthread_mutex_trylock(mtx)) == EBUSY) {
    gettimeofday(&now, NULL);

    if (now.tv_sec > ts->tv_sec ||
        (now.tv_sec == ts->tv_sec && (now.tv_usec * 1000) >= ts->tv_nsec)) {
      return thrd_timedout;
    }

    nanosleep(&sleeptime, NULL);
  }
#else
  if ((res = pthread_mutex_timedlock(mtx, ts)) == ETIMEDOUT) {
    return thrd_timedout;
  }
#endif
  return res == 0 ? thrd_success : thrd_error;
}

static inline int mtx_unlock(mtx_t *mtx) {
  return pthread_mutex_unlock(mtx) == 0 ? thrd_success : thrd_error;
}

/* ---- condition variables ---- */

static inline int cnd_init(cnd_t *cond) {
  return pthread_cond_init(cond, 0) == 0 ? thrd_success : thrd_error;
}

static inline void cnd_destroy(cnd_t *cond) { pthread_cond_destroy(cond); }

static inline int cnd_signal(cnd_t *cond) {
  return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_broadcast(cnd_t *cond) {
  return pthread_cond_broadcast(cond) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_wait(cnd_t *cond, mtx_t *mtx) {
  return pthread_cond_wait(cond, mtx) == 0 ? thrd_success : thrd_error;
}

static inline int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts) {
  int res;

  if ((res = pthread_cond_timedwait(cond, mtx, ts)) != 0) {
    return res == ETIMEDOUT ? thrd_timedout : thrd_error;
  }
  return thrd_success;
}

/* ---- thread-specific data ---- */

static inline int tss_create(tss_t *key, tss_dtor_t dtor) {
  return pthread_key_create(key, dtor) == 0 ? thrd_success : thrd_error;
}

static inline void tss_delete(tss_t key) { pthread_key_delete(key); }

static inline int tss_set(tss_t key, void *val) {
  return pthread_setspecific(key, val) == 0 ? thrd_success : thrd_error;
}

static inline void *tss_get(tss_t key) { return pthread_getspecific(key); }

/* ---- misc ---- */

#if 0  //__STDC_VERSION__ < 201112L || defined(C11THREADS_NO_TIMED_MUTEX)
/* TODO take base into account */
static inline int timespec_get(struct timespec *ts, int base)
{
  struct timeval tv;

  gettimeofday(&tv, 0);

  ts->tv_sec = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000;
  return base;
}
#endif /* not C11 */

#endif /* C11THREADS_H_ */
#endif // CUTILS_C11THREADS_H
