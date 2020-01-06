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
#ifndef CUTILS_KQUEUE_H
#define CUTILS_KQUEUE_H

#include <cutils/klist.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef bool (*kqueue_find_predicate_f)(KListElem *pElem, va_list argList );

typedef void (*kqueue_drop_f)(KListElem* pElem );
/**
 * @struct KQueue - a Queue implementation based on KList.
 * This is not thread safe.
 */
typedef struct _kqueue_t
{
  KListHead *p_head;
  KListTail *p_tail;
  kqueue_drop_f f_drop;
  uint32_t item_count;
}kqueue_t;

/**
 * @fn kqueue_init - Initializes the Queue.
 *
 *
 * @param pQueue - Allocated Queue owned by client.
 */
static inline void kqueue_init(kqueue_t* pQueue )
{
  if ( pQueue ) {
    pQueue->p_head = pQueue->p_tail = 0;
    pQueue->f_drop = 0;
    pQueue->item_count = 0;
  }
}

/**
 * @fn kqueue_is_empty - Queries if queue is empty
 *
 *
 * @param pQueue - Allocated and initialized Queue.
 *
 * @return bool - true if empty.
 */
static inline bool kqueue_is_empty(kqueue_t *pQueue )
{
  bool retval = true;
  if (pQueue && pQueue->p_head != 0 && pQueue->p_tail != 0 ) {
    retval = false;
  }
  return retval;
}

/**
 * @fn kqueue_insert  - Inserts an element into the tail of the queue.
 *
 *
 * @param p_queue - Allocated and initialized queue.
 * @param p_elem - element to insert.
 */
static inline void kqueue_insert(kqueue_t *p_queue, KListElem *p_elem )
{
  if (p_queue && p_elem ) {
    KLIST_TAIL_APPEND(p_queue->p_tail, p_elem );
    if (p_queue->p_head == 0 ) {
      p_queue->p_head = p_queue->p_tail;
    }
    p_queue->item_count++;
  }
}

/**
 * @fn kqueue_dequeue - Removes and element from the head of the
 * queue.
 *
 *
 * @param p_queue - ALlocated and initialized queue.
 *
 * @return KListElem* - Element that has been dequeued.
 */
static inline KListElem* kqueue_dequeue(kqueue_t *p_queue )
{
  KListElem *p_item = 0;
  if ( !kqueue_is_empty(p_queue) ) {
    if (p_queue->p_head == p_queue->p_tail ) {
      //Only one element
      p_queue->p_tail = 0;
    }
    p_item = p_queue->p_head;
    p_queue->p_head = p_item->next;
    KLIST_REMOVE_ELEM(p_item );
    p_queue->item_count--;
  }
  return p_item;
}

/**
 * @fn kqueue_register_drop_all_cb
 * Registers a callback on the queue
 * which is called on every item in the queue, when the function
 * KQueueDropAll is called on the queue. The client can use this
 * to cleanup individual entries if the Queue is dropped.
 *
 *
 * @param pQueue
 * @param cb
 */
static inline void kqueue_register_drop_all_cb(kqueue_t* pQueue, kqueue_drop_f cb )
{
  if ( pQueue ) {
    pQueue->f_drop = cb;
  }
}

/**
 * @fn kqueue_drop_all
 * This will drop all queued items. An optional
 * callback can be called when frames are dropped if one is
 * registered on the queue.
 *
 *
 * @param p_queue
 */
static inline void kqueue_drop_all(kqueue_t *p_queue )
{
  KListElem *pElem= 0;
  while( !kqueue_is_empty(p_queue) ) {
    pElem = kqueue_dequeue(p_queue);
    if ( p_queue->f_drop ) {
      p_queue->f_drop(pElem );
    }
  }
}

/**
 * @fn kqueue_find_first
 * Return first element (starting from head) for which FindPredicate function
 * evaluates to true
 *
 * @param p_queue
 * @param f_find
 */
static inline KListElem* kqueue_find_first(kqueue_t* p_queue, kqueue_find_predicate_f f_find, ... )
{
  va_list argList;
  va_start(argList, f_find );
  KListElem* pRetVal = 0;

  KListElem *pElem = p_queue->p_head;
  while( pElem ) {
    KListElem* pNext = pElem->next;
    if( f_find(pElem, argList ) ) {
      pRetVal = pElem;
      break;
    }
    pElem = pNext;
  }

  va_end( argList );
  return pRetVal;
}


/**
 * @fn kqueue_drop_all_through_elem - Delete all elements from head through
 *  to requested element
 *
 * @param p_queue
 * @param p_elem
 */
static inline void kqueue_drop_all_through_elem(kqueue_t *p_queue, KListElem *p_elem )
{
  KListElem *pDequeue= 0;
  while( true ) {
    pDequeue = kqueue_dequeue(p_queue);
    if ( !pDequeue ) {
      // reached end of list without finding anything just break
      break;
    } else if (p_elem == pDequeue ) {
      p_queue->f_drop(pDequeue );
      break;
    } else {
      p_queue->f_drop(pDequeue );
    }
  }
}

/**
 * @fn kqueue_get_item_count - Returns the number of elements in the
 * queue
 *
 * @param p_queue - pointer to a Valid initialized Queue
 * @return uint32_t - number of elements in queue.
 */
static inline uint32_t kqueue_get_item_count(kqueue_t* p_queue )
{
  uint32_t count = 0;
  if( p_queue ) {
    count = p_queue->item_count;
  }
  return count;
}

#ifdef __cplusplus
}
#endif

#endif //CUTILS_KQUEUE_H
