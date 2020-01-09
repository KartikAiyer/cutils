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

#include <cutils/os_types.h>
#include <cutils/ts_queue.h>
#include <stdalign.h>
#include <string.h>
#include <stdatomic.h>
#include <cutils/logger.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief This provides a static block pool implementation.  The pool allows the allocation of a fixed number of fixed
 * sized units specified at pool creation. Storage for the pool is provided using static storage and macros are provided
 * to create the storage needed for various pool instances
 */

/**
 * @brief Callback Signature for pool element destructor. Clients can install a destructor callback on individual
 * allocation out of a pool
 */
typedef void (*pool_element_destructor_f)(void *mem, void * private ) ;

typedef struct
{
  ts_queue_t *q;
  size_t num_of_elements;
  size_t element_size;
  size_t offset_header_from_data;
} pool_t;

typedef struct _pool_header_t
{
  uint32_t sanity;
  atomic_uint retain_count;
  pool_element_destructor_f destructor;
  void *destructor_private;
} pool_header_t;

#define POOL_STORE_TYPE(name) pool_static_backing_store_##name##_t
#define POOL_STORE_PTR_TYPE(name) POOL_STORE_TYPE( name )*
#define POOL_STORE(name) _pool_backing_store_##name

/**
 * @brief Storage for a pool is created statically. This is the first part of creating the storage needed to be used by a pool
 * Use this macro to create  a declaration for the specific pool storage that the client wishes to create.
 * For instance, if a client wants to create a pool with each element being the size of a ULONG and a total of 8 ULONGs
 * all aligned to 16 bytes, Storage will be declared using this macro.
 * ie POOL_STORE_DECL(someMacroIdentifier, 8, sizeof(ULONG), 16)
 */
#define POOL_STORE_DECL(name, num_elemens, element_size, align)\
TS_QUEUE_STORE_DECL(pool_##name, num_elemens);\
typedef struct\
{\
  struct {\
     pool_header_t header;\
     alignas(align) uint8_t data[ element_size ];\
     uint32_t trailer_sanity;\
  }elements[num_elemens];\
  pool_t pool;\
}POOL_STORE_TYPE( name )

/**
 * @brief Storage for a pool is created statically. This is the second part of creating the storage needed by a pool.
 * Use this macro after declaring a pool storage as above. Continuing with the above example,
 * POOL_STORE_DEF(someMacroIdentifier)
 */
#define POOL_STORE_DEF(name)\
POOL_STORE_TYPE( name ) POOL_STORE( name );\
TS_QUEUE_STORE_DEF(pool_##name)

typedef struct _pool_create_params_t
{
  pool_t *p_pool;
  uint32_t num_of_elements;
  uint32_t element_size_requested;
  uint32_t total_element_size;
  uint8_t *p_backing;
  uint32_t offset_header_from_data;
  ts_queue_create_params_t queue_params;
} pool_create_params_t;

/**
 * @brief This is the final part for creating a pool from static storage. Use this macro to create a pool_create_params_t
 * data structure used by pool_new() using the storage created using POOL_STORE_DEF(). Continuing with the example
 * {
 *   pool_create_params_t create_params;
 *   POOL_CREATE_INIT(params, someMacroIdentifier);
 *   pool_t *p_pool = pool_new(&create_params);
 * }
 */
#define POOL_CREATE_INIT(params, name)\
memset(&(params), 0, sizeof((params)));\
(params).p_pool = &POOL_STORE(name).pool;\
(params).num_of_elements = GetArraySize(POOL_STORE(name).elements);\
(params).element_size_requested = sizeof(POOL_STORE(name).elements[0].data);\
(params).total_element_size = sizeof(POOL_STORE(name).elements[0]);\
(params).p_backing = (uint8_t*)POOL_STORE(name).elements;\
(params).offset_header_from_data = (size_t)POOL_STORE(name).elements[0].data - (size_t)&POOL_STORE(name).elements[0].header;\
TS_QUEUE_STORE_CREATE_PARAMS_INIT((params).queue_params, pool_##name)

#define POOL_ELEMENT_HEADER_SANITY      (0xDEADBEEF)
#define POOL_ELEMENT_TRAILER_SANITY     (0xFACEB007)

/**
 * @brief Creates a new pool specified by the `create_params` specified. Use the static storage macros above
 * to create the create_params.
 * @param create_params - parameters specifying the attributes of the pool. Static storage must already be provided
 * @return - pointer to the created pool if successful, NULL otherwise
 */
static inline pool_t *pool_create(pool_create_params_t *create_params)
{
  pool_t *retval = 0;
  if (create_params->p_pool) {
    memset(create_params->p_pool, 0, sizeof(pool_t));
    create_params->p_pool->q = ts_queue_init(&create_params->queue_params);
    CHECK_RUN(create_params->p_pool->q, return retval, "%s(): Couldn't create static queue", __FUNCTION__);
    if (create_params->p_pool->q) {
      create_params->p_pool->num_of_elements = create_params->num_of_elements;
      create_params->p_pool->element_size = create_params->element_size_requested;
      create_params->p_pool->offset_header_from_data = create_params->offset_header_from_data;
      for (uint32_t i = 0; i < create_params->num_of_elements; i++) {
        uint8_t *data =
            create_params->p_backing + (i * create_params->total_element_size) + create_params->offset_header_from_data;
        pool_header_t *p_header = (pool_header_t*)(data - create_params->offset_header_from_data);
        uint32_t *p_trailer_sanity;
        memset(p_header, 0, sizeof(pool_header_t));
        p_header->sanity = POOL_ELEMENT_HEADER_SANITY;
        atomic_init(&p_header->retain_count, 0);
        p_trailer_sanity = (uint32_t*)data + create_params->element_size_requested;
        *p_trailer_sanity = POOL_ELEMENT_TRAILER_SANITY;
        ts_queue_enqueue(create_params->p_pool->q, data, NO_SLEEP);
      }
      retval = create_params->p_pool;
    }
  }
  return retval;
}
/**
 * @brief Will release the resources in a pool. Clients should not use the pool after freeing with this
 * @param p_pool - A valid allocated pool
 */
static inline void pool_destroy(pool_t *p_pool)
{
  if (p_pool) {
    ts_queue_destroy(p_pool->q);
    p_pool->q = 0;
  }
}

/**
 * @brief Will allocate a fixed size block from the pool. Will block for `wait_ms` if the pool is empty.
 * @param p_pool - Created and valid pool
 * @param wait_ms - number of ticks to sleep if queue empty.
 * @param fnDestroy - An optional destructor  to be used with the allocation
 * @param destructor_private - Private Client data to be suplplied with the destructor
 * @return - a block allocated from the pool if successful, NULL otherwise
 */
static inline void *
pool_alloc_blocking(pool_t *p_pool, uint32_t wait_ms, pool_element_destructor_f fnDestroy, void *destructor_private)
{
  void *retval = 0;
  if (p_pool) {
    pool_header_t *p_header = 0;
    if (ts_queue_dequeue(p_pool->q, &retval, wait_ms)) {
      p_header = (pool_header_t*)((uint8_t*)retval - p_pool->offset_header_from_data);
      CUTILS_ASSERT(p_header->sanity == POOL_ELEMENT_HEADER_SANITY);
      CUTILS_ASSERT(*((uint32_t *) (retval + p_pool->element_size)) == POOL_ELEMENT_TRAILER_SANITY);
      atomic_fetch_add_explicit(&p_header->retain_count, 1, memory_order_relaxed);
      if (fnDestroy) {
        p_header->destructor = fnDestroy;
        p_header->destructor_private = destructor_private;
      }
    }
  }
  return retval;
}

/**
 * @brief A conveneience function that is non-blocking and does not specify a destructor
 * @param p_pool - A Valid Pool
 * @return - a block allocated from the pool if successful, NULL otherwise
 */
static inline void *pool_alloc(pool_t *p_pool)
{ return pool_alloc_blocking(p_pool, NO_SLEEP, NULL, NULL); }

/**
 * @brief Increases the reference count of an allocation from the specified pool
 * @param p_pool - a Valid Pool
 * @param p_mem - A previously allocated block from the supplied pool
 */
static inline void pool_retain(pool_t *p_pool, void *p_mem)
{
  if (p_pool) {
    pool_header_t *p_header = p_mem - p_pool->offset_header_from_data;
    CUTILS_ASSERT(p_header->sanity == POOL_ELEMENT_HEADER_SANITY);
    CUTILS_ASSERT(*((uint32_t *) (p_mem + p_pool->element_size)) == POOL_ELEMENT_TRAILER_SANITY);
    atomic_fetch_add_explicit(&p_header->retain_count, 1, memory_order_relaxed);
  }
}

/**
 * @brief - Returns the supplied block back to the pool if the reference count hits zero. Otherwise simply
 * lowers the reference count of the supplied allocation. Clients should not use the supplied memory block after
 * calling this function on the block.
 * @param p_pool - a valid pool
 * @param p_mem - Block Allocation to return to the pool
 */
static inline void pool_free(pool_t *p_pool, void *p_mem)
{
  if (p_pool) {
    pool_header_t *p_header = p_mem - p_pool->offset_header_from_data;
    uint32_t old_retain_count;
    CUTILS_ASSERT(p_header->sanity == POOL_ELEMENT_HEADER_SANITY);
    CUTILS_ASSERT(*((uint32_t *) (p_mem + p_pool->element_size)) == POOL_ELEMENT_TRAILER_SANITY);
    old_retain_count = atomic_fetch_sub_explicit(&p_header->retain_count, 1, memory_order_acq_rel);
    CUTILS_ASSERT(old_retain_count != 0);
    if (old_retain_count == 1) {
      if (p_header->destructor) p_header->destructor(p_mem, p_header->destructor_private);
      ts_queue_enqueue(p_pool->q, p_mem, NO_SLEEP);
    }
  }
}

/**
 * @brief Allows the client to set a destructor callback on an allocation from a pool.
 * @param p_pool - a valid pool
 * @param p_mem - previously allocated block from the pool
 * @param fnDestroy - A destructor to be called when the allocation is released to the pool
 * @param destructor_private - Client data to be supplied to the destructor callback.
 */
static inline void
pool_set_destructor(pool_t *p_pool, void *p_mem, pool_element_destructor_f fnDestroy, void *destructor_private)
{
  if (p_pool && p_mem) {
    pool_header_t *p_header = p_mem - p_pool->offset_header_from_data;
    CUTILS_ASSERT(p_header->sanity == POOL_ELEMENT_HEADER_SANITY);
    CUTILS_ASSERT(*((uint32_t *) (p_mem + p_pool->element_size)) == POOL_ELEMENT_TRAILER_SANITY);
    p_header->destructor = fnDestroy;
    p_header->destructor_private = destructor_private;
  }
}
#ifdef __cplusplus
}
#endif
