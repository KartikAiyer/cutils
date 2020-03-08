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

#include <cutils/notifier.h>


#define NOTIF_CLOG(str, ...)        console_log(&notifier->log.base, NULL, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define CLOGV(str, ...)       CLOG_IF(notifier->log.base.log_level > 0, str, ##__VA_ARGS__)
#define CLOGV_FIELD(x, f)     LOG_FIELD(x,f,CLOGV)

bool notifier_init(notifier_create_params_t *params)
{
  bool retval = false;

  CUTILS_ASSERTF(params && params->notifier && params->notif_array, "Bad Inputs")
  notifier_t *notifier = params->notifier;

  memset(notifier, 0, sizeof(notifier_t));
  notifier->notif_array = params->notif_array;
  notifier->total_notification_categories = params->total_categories;
  notifier->log = params->log;
  notifier->execute_f = params->execute_f;

  CLOGV("");
  CLOGV_FIELD(params->max_registrations, u);
  CLOGV_FIELD(params->total_categories, u);
  CLOGV_FIELD(params->notif_block_size, u);
  CLOGV_FIELD(params->notifier, p);
  CLOGV_FIELD(params->pool_params.num_of_elements, u);
  CLOGV_FIELD(params->pool_params.element_size_requested, u);

  if (mutex_new(&notifier->mtx)) {
    notifier->p_pool = pool_create(&params->pool_params);
    if (notifier->p_pool) {
      uint32_t i = 0;
      for (i = 0; i < notifier->total_notification_categories; i++) {
        notifier->notif_array[i] = 0;
      }
      retval = true;
    } else {
      NOTIF_CLOG("Couldn't Create Pool");
      mutex_free(&notifier->mtx);
    }
  }
  CLOGV("%s(): Retval = %u", __FUNCTION__, retval);
  return retval;
}

void notifier_deinit(notifier_t *notifier)
{
  CUTILS_ASSERT(notifier);
  pool_destroy(notifier->p_pool);
  notifier->p_pool = 0;
  mutex_free(&notifier->mtx);
}

notifier_block_t *notifier_allocate_notification_block(notifier_t *notifier)
{
  notifier_block_t *retval = 0;

  CUTILS_ASSERT(notifier);
  mutex_lock(&notifier->mtx, WAIT_FOREVER);
  retval = pool_alloc(notifier->p_pool);
  CUTILS_ASSERTF(retval, "Couldn't Allocate Notification Block, Pool = %p", notifier->p_pool);
  mutex_unlock(&notifier->mtx);
  memset(retval, 0, notifier->p_pool->element_size);
  return retval;
}

bool notifier_register_notification_block(notifier_t *notifier, uint32_t category, notifier_block_t *block)
{
  CUTILS_ASSERT(notifier && block && category < notifier->total_notification_categories);
  mutex_lock(&notifier->mtx, WAIT_FOREVER);
  block->category = category;
  KLIST_HEAD_PREPEND(notifier->notif_array[category], block);
  mutex_unlock(&notifier->mtx);
  return true;
}

void notifier_deregister_notification_block(notifier_t *notifier, notifier_block_t *block)
{
  CUTILS_ASSERT(notifier && block && block->category < notifier->total_notification_categories)
  mutex_lock(&notifier->mtx, WAIT_FOREVER);
  if (notifier->notif_array[block->category] == (KListHead *) block) {
    notifier_block_t *pPopped;

    KLIST_HEAD_POP(notifier->notif_array[block->category], pPopped);
    CUTILS_ASSERT(pPopped == block);
  } else {
    KLIST_REMOVE_ELEM(block);
  }
  pool_free(notifier->p_pool, block);
  mutex_unlock(&notifier->mtx);
}

static void Poster(KListElem *elem, ...)
{
  va_list args;
  notifier_t *notifier = 0;
  void *pNotificationData = 0;
  notifier_block_t *pBlock = (notifier_block_t *) elem;

  va_start(args, elem);
  notifier = va_arg(args, notifier_t *);
  pNotificationData = va_arg(args, void *);

  CUTILS_ASSERTF(notifier, "Notifier Failure. Expected valid pointer!!!");
  if (notifier) {
    notifier->execute_f(pBlock, pBlock->category, pNotificationData);
  }
  va_end(args);
}

void notifier_post_notification(notifier_t *notifier, uint32_t category, void *notification_data)
{
  CUTILS_ASSERTF(notifier && category < notifier->total_notification_categories, "Notifier: %p, category: %d, tot: %d",
      notifier, category, notifier->total_notification_categories );
  mutex_lock(&notifier->mtx, WAIT_FOREVER);
  KLIST_HEAD_FOREACH(notifier->notif_array[category], Poster, notifier, notification_data);
  mutex_unlock(&notifier->mtx);
}
