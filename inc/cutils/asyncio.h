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

#include <cutils/dispatch_queue.h>
#include <cutils/event_flag.h>

/**
 * @subsection asyncio_interface_t - The Stream Interface provides
 *             interfaces to work with serial streams where the
 *             unit of the transaction is a Message of arbitrary
 *             size. The messages are size delimited and output
 *             on the wire using a four byte size header.
 *
 *
 */

typedef void *asyncio_handle_t;
typedef uint8_t *asyncio_message_t;
typedef void *asyncio_tx_token_t;

#define STREAM_TX_SEND_STATUS(ACTION)\
  ACTION(StreamTxSendSuccess,)\
  ACTION(StreamTxSendHeaderFail,)\
  ACTION(StreamTxSendMessageFail,)\
  ACTION(StreamTxInterfaceInError,)

DECLARE_ENUM(asyncio_tx_send_status_e, STREAM_TX_SEND_STATUS)

/**
 * asyncio_rx_callback_f - Called when a delimited message is recd on
 * the stream interface. This task should not block and should not take
 * long. Copy the data out or process and immediately. Anything
 * heavy should be scheduled on another task.
 *
 * @param pMessage - pointer to recd. message.
 * @param size - size of message
 * @param pPrivate - Private data provided at time of
 *                 registration.
 */
typedef void (*asyncio_rx_callback_f)(asyncio_handle_t h_asyncio, asyncio_message_t pMessage, size_t size);

/**
 * asyncio_tx_notification_f - Called when a set number of bytes has
 * been sent via the stream interface. The callback should not go any heavy
 * lifting in this context.
 *
 * @param hMessage - original pointer to message,
 * @param sendStatus - StreamTxSendSuccess if message was sent via stream interface
 *                   successfully, StreamTxSendHeaderFail or StreamTxSendMessageFail
 *                   if header or message send failed
 * @param sendSize - number of bytes sent.
 * @param pPrivate  - private data at time of registration
 */
typedef void (*asyncio_tx_notification_f)(asyncio_tx_token_t token,
                                          asyncio_tx_send_status_e sendStatus,
                                          uint32_t sendSize,
                                          void *pPrivate);

/**
 * asyncio_interface_client_read_f - Client specific Read implementation. This call should
 * make a blocking call on the interface for the Read.
 *
 * @param handle - Stream handle
 * @param pRxDataBuf - Data buffer that the client will read into. Size is prefixed max size
 * @param timeOut - in ms.
 *
 * @return size_t - Number of bytes read
 */
typedef size_t (*asyncio_interface_client_read_f)(asyncio_handle_t handle,
                                                  uint8_t *pRxDataBuf,
                                                  uint32_t timeOut);

/**
 * asyncio_interface_client_write_f - Client specific Write implementation. This call should
 * make a blocking call on the interface and write out contents of supplied buffer.
 *
 * @param handle - Stream handle
 * @param txDataSize - transmit data size
 * @param pTxDataBuf - Data buffer that the client will write out off.
 * @param timeOut - in ms.
 *
 * @return size_t - Number of bytes written
 */

typedef size_t(*asyncio_interface_client_write_f)(asyncio_handle_t handle,
                                                  size_t tx_data_size,
                                                  uint8_t *p_tx_data_buf,
                                                  uint32_t timeout);

#define ASYNCIO_MAX_THREAD_NAME              ( 40 )
#define ASYNCIO_SERVICE_UNUSED               (0)


#define ASYNCIO_INIT_STATE_E(XX)\
  XX(AsyncioUninitialized,)\
  XX(AsyncioUninitializing,)\
  XX(AsyncioInitializing, )\
  XX(AsyncioInitialized, )

DECLARE_ENUM(asyncio_init_state_e, ASYNCIO_INIT_STATE_E)

typedef struct _asyncio_rx_thread_context
{
  dispatch_queue_t *worker;
  pool_create_params_t rx_pool_params;
  pool_t *buffer_pool;
  char thread_name[ASYNCIO_MAX_THREAD_NAME];
  asyncio_rx_callback_f rx_callback;
  void *client_data;
  atomic_int init_state;
} asyncio_rx_thread_context_t;

typedef struct _asyncio_tx_request
{
  uint32_t size;
  uint32_t buffer_offset;
  asyncio_tx_notification_f fn;
  void *p_private;
} asyncio_tx_request_t;

typedef struct _asyncio_tx_thread_context_t
{
  dispatch_queue_t *worker;
  pool_create_params_t pool_create_params;
  char thread_name[ASYNCIO_MAX_THREAD_NAME];
  pool_t *buffer_pool;
  uint32_t max_tx_data_chunk_size;
  uint32_t tx_write_timeout;
  uint32_t tx_buffer_offset;
  atomic_int init_state;
} asyncio_tx_thread_context_t;

typedef struct _asyncio_interface_instance_t
{
  asyncio_rx_thread_context_t rx_task;
  asyncio_tx_thread_context_t tx_task;
  asyncio_interface_client_read_f f_stream_interface_read;
  asyncio_interface_client_write_f f_stream_interface_write;
  logger_t log;
  bool is_in_error;
  bool stream_stopped;
  const char *stream_name;
  event_flag_t event_flag;
  atomic_int init_state;
} asyncio_interface_instance_t;

typedef struct _asyncio_create_params_t
{
  const char *stream_name;

  // Client callbacks and parameters
  asyncio_interface_instance_t *p_instance;
  asyncio_interface_client_read_f read_f;
  asyncio_interface_client_write_f write_f;
  asyncio_rx_callback_f rx_callback;
  void *client_data;

  // rx param
  dispatch_queue_t *rx_worker;
  pool_create_params_t rx_pool_create_params;

  // tx params
  dispatch_queue_t *tx_worker;
  pool_create_params_t tx_pool_create_params;
  uint32_t tx_max_chunk_size;
  uint32_t tx_write_timeout;
  uint32_t tx_buffer_offset;
} asyncio_create_params_t;

#define ASYNCIO_STORE(name)       _asyncio_store_##name
#define ASYNCIO_STORE_T(name)     _asyncio_store_##name##_t

#define ASYNCIO_STORE_DECL(name, rx_max_buffers, tx_max_buffers, rx_max_buf_size, tx_max_buf_size, buffer_align)\
typedef struct {\
  asyncio_tx_request_t request;\
  uint8_t payload[ (tx_max_buf_size)?tx_max_buf_size:buffer_align ] __attribute__((aligned(buffer_align)));\
}__asyncio_##name##_tx_message_t;\
POOL_STORE_DECL( name##_tx, ((tx_max_buffers)?tx_max_buffers:1), sizeof(__asyncio_##name##_tx_message_t), buffer_align);\
POOL_STORE_DECL( name##_rx, ((rx_max_buffers)?rx_max_buffers:1), (rx_max_buf_size)?rx_max_buf_size:buffer_align, buffer_align );\
typedef struct {\
  asyncio_interface_instance_t instance;\
}ASYNCIO_STORE_T( name );\
static uint32_t __tx_max_buf_size_##name = (tx_max_buf_size)?tx_max_buf_size:buffer_align;

#define ASYNCIO_STORE_DEF(name)\
POOL_STORE_DEF(name##_tx);\
POOL_STORE_DEF(name##_rx);\
ASYNCIO_STORE_T( name ) ASYNCIO_STORE( name ) = { 0 }

#define ASYNCIO_CREATE_PARAMS_INIT(params, name, st_name, read_fn, write_fn, rx_cb, rx_dq, tx_dq, p_priv) do{\
  (params).stream_name = st_name;\
  (params).p_instance = &ASYNCIO_STORE( name ).instance;\
  (params).read_f = read_fn;\
  (params).write_f = write_fn;\
  (params).rx_callback = rx_cb;\
  (params).client_data = p_priv;\
  (params).rx_worker = rx_dq,\
  POOL_CREATE_INIT((params).rx_pool_create_params, name##_rx);\
  (params).tx_worker = tx_dq;\
  POOL_CREATE_INIT((params).tx_pool_create_params, name##_tx);\
  (params).tx_buffer_offset = offsetof(__asyncio_##name##_tx_message_t, payload);\
  (params).tx_max_chunk_size = __tx_max_buf_size_##name;\
  (params).tx_write_timeout = 4000;\
}while( 0 )

asyncio_handle_t asyncio_create_instance(asyncio_create_params_t *p_params);

void asyncio_destroy_instance(asyncio_handle_t h_asyncio);

bool asyncio_start(asyncio_handle_t h_asyncio);

void asyncio_stop(asyncio_handle_t h_asyncio);

asyncio_tx_token_t asyncio_allocate_tx_token(asyncio_handle_t h_asyncio);

uint8_t *asyncio_tx_token_get_data_buffer(asyncio_tx_token_t token);

uint32_t asyncio_tx_token_get_max_data_size(asyncio_handle_t h_asyncio);

bool asyncio_send_buffer(asyncio_handle_t h_asyncio,
                         asyncio_tx_token_t token,
                         uint32_t size, asyncio_tx_notification_f fn, void *p_private);

void asyncio_release_rx_buffer(asyncio_handle_t h_asyncio, asyncio_message_t message);

void asyncio_release_tx_token(asyncio_handle_t h_asyncio, asyncio_tx_token_t token);

void *asyncio_get_private_data(asyncio_handle_t h_asyncio);

