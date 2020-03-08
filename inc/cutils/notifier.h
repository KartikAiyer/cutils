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

#include <cutils/types.h>
#include <cutils/logger.h>
#include <cutils/mutex.h>
#include <cutils/klist.h>
#include <cutils/pool.h>

/**
 * @file Notifier.h
 * @brief Category specific notifier.
 *   
 *  The Notifier API provides a way to store Notification
 *  callbacks for different types of notifications. A table of
 *  Notification callbacks is maintained. THe Table is indexed
 *  by the notification type and the value is alist of
 *  callbacks. When a notification is posted, the callbacks in
 *  the corresponding list are called one by one. 
 */

/**
 * @struct NotifierBlock
 *  
 * @brief Different Notifications should use this as the first
 *        element in their notification data structure.
 *  
 * This is used to queue notifications into the lists 
 * internally. This is meant to be used by a client that want to 
 * offer specific notifications for events that it handles. 
 */
typedef struct
{
  KListElem elem;
  uint32_t category;
} notifier_block_t;

/**
 * @fn notifier_f
 * @brief callback for a specific notification.
 *  
 * Clients can register callbacks of this type. 
 *  
 * @param pBlock - The Notification that was posted will be
 *        called
 * @param category - the notification type that is used as a key
 *        to the callback table.
 * @param pNotificationData - any private data that is posted in
 *        the notification
 */
typedef void (*notifier_f) (notifier_block_t * pBlock, uint32_t category, void *pNotificationData);

/**
 * This is the basic Notifier data structure that clients that 
 * want to expose a notification system, must initialize. 
 */
typedef struct _notifier_t
{
  mutex_t mtx;
  KListHead **notif_array;
  uint32_t total_notification_categories;
  notifier_f execute_f;
  pool_t *p_pool;
  simple_logger_t log;
} notifier_t;

/**
 * Createion data structure used to encapsulate all inputs 
 * needed to create a notifier. 
 */
typedef struct _notifier_create_params_t
{
  notifier_t *notifier;  /**< The data for the notifier must be provided by the client. This will be initialized by NotifierInit */
  uint32_t notif_block_size; /**< This is the size of the data structure that represents a Notifier Block that is registered with a notification */
  uint32_t max_registrations; /**< The maximum number of Notifications that can be registered.*/
  uint32_t total_categories; /**< The total number of notification categories.*/
  KListHead **notif_array; /**< The backing store to be used to save registered notifications */
  pool_create_params_t pool_params;
  notifier_f execute_f; /**< This is the Client specific notification callback.*/
  simple_logger_t log;
} notifier_create_params_t;

#define NOTIFIER_STORE_T( name )     _notifier_store_##name##_t
#define NOTIFIER_STORE( name )       _notifier_store_##name

#define NOTIFIER_STORE_DECL( name, num_of_cat, max_regs, notif_reg_size)\
POOL_STORE_DECL(notif_pool_##name, max_regs, notif_reg_size, 4);\
typedef struct {\
  KListHead* notifierArray[ num_of_cat ];\
  notifier_t not;\
}NOTIFIER_STORE_T( name );\
size_t _notif_registration_size_##name = notif_reg_size;\
uint32_t _notif_num_of_registrations_##name = max_regs

#define NOTIFIER_STORE_DEF( name )\
NOTIFIER_STORE_T( name ) NOTIFIER_STORE( name );\
POOL_STORE_DEF(notif_pool_##name)


// Use this macro to initialize a notifier_create_params_t if you created storage using the above DECL/DEF
// macros.
#define NOTIFIER_STORE_CREATE_PARAMS_INIT( params, name, notif_f, notif_name, should_log )\
memset(&(params), 0, sizeof((params)));\
(params).notifier = &NOTIFIER_STORE(name).not;\
(params).notif_block_size = _notif_registration_size_##name;\
(params).max_registrations = _notif_num_of_registrations_##name;\
(params).total_categories = GetArraySize(NOTIFIER_STORE(name).notifierArray);\
(params).notif_array = NOTIFIER_STORE(name).notifierArray;\
memset(NOTIFIER_STORE(name).notifierArray, 0, sizeof(NOTIFIER_STORE(name).notifierArray));\
POOL_CREATE_INIT((params).pool_params, notif_pool_##name);\
(params).execute_f = notif_f;\
(params).log = simple_logger_init(notif_name, should_log)

/**
 * NotifierInit - Creates a notifier based on the creation 
 * parameters supplied. 
 * 
 * \param pParams 
 * 
 * \return bool - true if successfully created. 
 */
bool notifier_init(notifier_create_params_t * params);

/**
 * NotifierDeInit - DeInitializes a notifier. All registered 
 * notifications will be removed and resources cleaned up. 
 * 
 * \param pNotifier 
 */
void notifier_deinit(notifier_t * notifier);

/**
 * NotifierAllocateNotificationBlock - Allocates a Notification 
 * block. The clients use this block to fill up the specific 
 * notification data and then registers with the notifier. 
 * 
 * \param pNotifier 
 * 
 * \return NotifierBlock* 
 */
notifier_block_t *notifier_allocate_notification_block(notifier_t * notifier);

/**
 * NotifierRegisterNotificationBlock - Used to register a 
 * specific notification. 
 * 
 * \param pNotif 
 * \param category - category for the notification.
 * \param pBlock 
 * 
 * \return bool 
 */
bool notifier_register_notification_block(notifier_t * notifier, uint32_t category, notifier_block_t * block);

/**
 * NotifierDeRegisterNotificationBlock - Removes a specific 
 * notification. 
 * 
 * \param pNotif 
 * \param pBlock 
 */
void notifier_deregister_notification_block(notifier_t * notifier, notifier_block_t * block);

/**
 * NotifierPostNotification - Posts a notification and calls any 
 * callbacks that are registered for the specific notrification 
 * 
 * \param pNotif 
 * \param category 
 * \param pNotificationData 
 */
void notifier_post_notification(notifier_t * notifier, uint32_t category, void *notification_data);
