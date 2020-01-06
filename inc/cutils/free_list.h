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


#ifndef CUTILS_FREE_LIST_H
#define CUTILS_FREE_LIST_H

#include <cutils/klist.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  KListHead *head;
  void* data_store;
  size_t item_size;
  size_t item_count;
  size_t current_available;
}free_list_t;

#define FREE_LIST_STORE(name)  _free_list_store_##name
#define FREE_LIST_STORE_T(name) _free_list_store_##name##_t
#define FREE_LIST_STORE_DECL(name, type, count)\
typedef struct {\
  type store[count];\
  free_list_t list;\
}FREE_LIST_STORE_T(name)

#define FREE_LIST_STORE_DEF(name)\
FREE_LIST_STORE_T(name) FREE_LIST_STORE(name)

typedef struct {
  free_list_t *list;
  void* data_store;
  size_t item_size;
  size_t item_count;
}free_list_create_params_t;

#define FREE_LIST_STORE_CREATE_PARAMS_INIT(params, name)\
(params).list = &FREE_LIST_STORE(name).list;\
(params).data_store = (void*)FREE_LIST_STORE(name).store;\
(params).item_size = sizeof(FREE_LIST_STORE(name).store[0]);\
(params).item_count = GetArraySize(FREE_LIST_STORE(name).store);

static inline free_list_t* free_list_init(free_list_create_params_t *p_params)
{
  free_list_t *retval = 0;
  if(p_params->list && p_params->data_store && p_params->item_size > 0) {
    p_params->list->data_store = p_params->data_store;
    p_params->list->item_size = p_params->item_size;
    p_params->list->item_count = p_params->item_count;
    p_params->list->head = 0;
    p_params->list->current_available = 0;
    for(size_t i = 0; i < p_params->item_count; i++) {
      KLIST_HEAD_PREPEND(p_params->list->head, ((char*)p_params->data_store + (p_params->item_size * i)));
      p_params->list->current_available++;
    }
    retval = p_params->list;
  }
  return retval;
}

static inline KListElem* free_list_get(free_list_t *list)
{
  KListElem *item = 0;
  if(list->head && list->current_available > 0) {
    KLIST_HEAD_POP(list->head, item);
    list->current_available--;
  }
  return item;
}

static inline bool free_list_put(free_list_t* list, KListElem *item)
{
  bool retval = false;
  if(list && list->current_available < list->item_count) {
    KLIST_HEAD_PREPEND(list->head, item);
    list->current_available++;
    retval = true;
  }
  return retval;
}

#ifdef __cplusplus
};
#endif
#endif //CUTILS_FREE_LIST_H
