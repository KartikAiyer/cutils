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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DACUTILSS OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cutils/asyncio.h>
#include <cutils/logger.h>
#include <stdatomic.h>

// Tx Dispatch action prototype
static void tx_f(void *arg1, void *arg2);
// Rx Dispatch action Prototype
static void rx_f(void* arg1, void* arg2);

// Other static Prototypes

static void asyncio_stop_rx_thread(asyncio_interface_instance_t *p_asyncio);
static void asyncio_stop_tx_thread(asyncio_interface_instance_t *p_asyncio);

static void asyncio_configure_tx_thread(asyncio_create_params_t *p_params,
                                        asyncio_interface_instance_t *p_asyncio);

static void asyncio_configure_rx_thread(asyncio_create_params_t *p_params,
                                        asyncio_interface_instance_t *p_asyncio);

static bool asyncio_start_tx_thread(asyncio_interface_instance_t *p_asyncio);

static bool asyncio_start_rx_thread(asyncio_interface_instance_t *p_asyncio);

//TODO: Enable this when System log has been integrated
#define STREAM_SLOG(str, ...)
//system_log( &p_asyncio->log, NULL, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define STREAM_CLOG(str, ...)\
console_log( &p_asyncio->log, NULL, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define  STREAM_CLOGV(str, ...)\
LOG_IF(STREAM_CLOG, p_asyncio->log.log_level, str, ##__VA_ARGS__ )

#define STREAM_STARTED_FLAG       (1 << 0)
#define STREAM_STOPPED_FLAG       (1 << 1)

static uint32_t asyncio_logger_prefix(logger_t* logger, void *client_data, char *string, uint32_t string_size)
{
  uint32_t retval = 0;
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t*)((size_t)logger - offsetof(asyncio_interface_instance_t, log));

  if( string_size > 30) {
    INSERT_TO_STRING(string, retval, "%s:%s(Tx:%s,Rx:%s)",
        p_asyncio->stream_name,
        get_asyncio_init_state_e_string(atomic_load(&p_asyncio->init_state)),
        get_asyncio_init_state_e_string(atomic_load(&p_asyncio->tx_task.init_state)),
        get_asyncio_init_state_e_string(atomic_load(&p_asyncio->rx_task.init_state)));
  }
  return retval;
}

asyncio_handle_t asyncio_create_instance(asyncio_create_params_t *p_params)
{
  asyncio_interface_instance_t *p_asyncio = NULL;
  CUTILS_ASSERTF(p_params->p_instance &&
               ((p_params->read_f && p_params->rx_callback) || p_params->write_f) &&
               ((p_params->read_f && p_params->rx_callback) || (!p_params->read_f && !p_params->rx_callback)),
               "Invalid stream creation parameters, Inst = %p, read_f = %p, rx_cb = %p, write_f = %p",
               p_params->p_instance, p_params->read_f, p_params->rx_callback, p_params->write_f);

  CUTILS_ASSERTF((p_params->rx_worker && p_params->read_f) || (p_params->tx_worker && p_params->write_f),
               "Appropriate Dispatch Queues not provided, rx_dq: %p, read_f: %p, tx_dq: %p, write_f: %p",
               p_params->rx_worker, p_params->read_f, p_params->tx_worker, p_params->write_f);
  p_asyncio = p_params->p_instance;

  CUTILS_ASSERTF(event_flag_new(&p_asyncio->event_flag), "Couldn't create event flag");
  p_asyncio->is_in_error = false;
  p_asyncio->stream_stopped = true;
  p_asyncio->f_stream_interface_read = p_params->read_f;
  p_asyncio->f_stream_interface_write = p_params->write_f;
  p_asyncio->stream_name = p_params->stream_name;
  p_asyncio->log.fnPrefix = asyncio_logger_prefix;
  p_asyncio->log.log_level = 0;
  p_asyncio->log.isEnabled = true;
  asyncio_configure_rx_thread(p_params, p_asyncio);
  asyncio_configure_tx_thread(p_params, p_asyncio);
  return p_asyncio;
}

void asyncio_destroy_instance(asyncio_handle_t h_asyncio)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  if (p_asyncio) {
    asyncio_stop(p_asyncio);
    event_flag_free(&p_asyncio->event_flag);
  } else {
    STREAM_SLOG("Failed: %s", get_asyncio_init_state_e_string(atomic_load(&p_asyncio->init_state)));
  }
}

bool asyncio_start(asyncio_handle_t h_asyncio)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  bool retval = true;
  int expected = AsyncioUninitialized;

  if (atomic_compare_exchange_strong(&p_asyncio->init_state, &expected, AsyncioInitializing)) {
    event_flag_clear(&p_asyncio->event_flag, STREAM_STARTED_FLAG);
    if (p_asyncio->rx_task.worker) {
      if (asyncio_start_rx_thread(p_asyncio)) {
        retval = true;
      } else {
        STREAM_SLOG("Couldn't Start Thread %s", p_asyncio->rx_task.thread_name);
        STREAM_CLOGV("Couldn't Start Thread %s", p_asyncio->rx_task.thread_name);
        retval = false;
      }
    }

    if (p_asyncio->tx_task.worker) {
      if (retval && asyncio_start_tx_thread(p_asyncio)) {
        retval = true;
      } else {
        STREAM_SLOG("Couldn't Start Thread %s", p_asyncio->tx_task.thread_name);
        STREAM_CLOGV("Couldn't Start Thread %s", p_asyncio->tx_task.thread_name);
        retval = false;
      }
    }

    p_asyncio->stream_stopped = false;
    if (retval) {
      CUTILS_ASSERTF(event_flag_wait(&p_asyncio->event_flag, STREAM_STARTED_FLAG, WAIT_OR, NULL, 4000),
                   "Failed to wait on Started Flag");
      STREAM_SLOG("Started");
      STREAM_CLOGV("Started");
    } else {
      atomic_store(&p_asyncio->init_state, AsyncioUninitialized);
    }
  } else {
    CLOG("Init State is bad %s", get_asyncio_init_state_e_string(atomic_load(&p_asyncio->init_state)));
    retval = false;
  }
  return retval;
}

void asyncio_stop(asyncio_handle_t h_asyncio)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  int expected = AsyncioInitialized;
  if (atomic_compare_exchange_strong(&p_asyncio->init_state, &expected, AsyncioUninitializing)) {
    if (!p_asyncio->stream_stopped) {
      event_flag_clear(&p_asyncio->event_flag, STREAM_STOPPED_FLAG);
      p_asyncio->stream_stopped = true;
      asyncio_stop_rx_thread(p_asyncio);
      asyncio_stop_tx_thread(p_asyncio);
      CUTILS_ASSERTF(event_flag_wait(&p_asyncio->event_flag, STREAM_STOPPED_FLAG, WAIT_OR, NULL, 4000),
          "Failed to wait on Stopped Flag");
      STREAM_SLOG("Stopped");
      STREAM_CLOGV("Stopped");
    }
  } else {
    STREAM_SLOG("Failed: State = %s", get_asyncio_init_state_e_string(atomic_load(&p_asyncio->init_state)));
  }
}

asyncio_tx_token_t asyncio_allocate_tx_token(asyncio_handle_t h_asyncio)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  asyncio_tx_thread_context_t *pCtx = &p_asyncio->tx_task;
  asyncio_tx_request_t *pReq = 0;

  if(atomic_load(&pCtx->init_state) == AsyncioInitialized) {
    pReq = (asyncio_tx_request_t *) pool_alloc(pCtx->buffer_pool);
    if (pReq) {
      pReq->fn = 0;
      pReq->p_private = 0;
      pReq->size = 0;
      pReq->buffer_offset = p_asyncio->tx_task.tx_buffer_offset;
    }
  }
  return (asyncio_tx_token_t) pReq;
}

void asyncio_release_tx_token(asyncio_handle_t h_asyncio, asyncio_tx_token_t token)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  asyncio_tx_thread_context_t *p_ctx = &p_asyncio->tx_task;

  if (p_asyncio && token && atomic_load(&p_ctx->init_state) == AsyncioInitialized) {
    pool_free(p_ctx->buffer_pool, token);
  }
}

static inline uint8_t *asyncio_tx_request_get_buffer(asyncio_tx_request_t *p_req)
{
  uint8_t *retval = 0;

  retval = ((uint8_t *) p_req) + p_req->buffer_offset;
  return retval;
}

uint8_t *asyncio_tx_token_get_data_buffer(asyncio_tx_token_t token)
{
  uint8_t *retval = 0;
  asyncio_tx_request_t *p_req = (asyncio_tx_request_t *) token;

  if (p_req) {
    retval = asyncio_tx_request_get_buffer(p_req);
  }
  return retval;
}

uint32_t asyncio_tx_token_get_max_data_size(asyncio_handle_t h_asyncio)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;

  return p_asyncio->tx_task.max_tx_data_chunk_size;
}

bool asyncio_send_buffer(asyncio_handle_t h_asyncio,
                         asyncio_tx_token_t token,
                         uint32_t size,
                         asyncio_tx_notification_f fn,
                         void *p_private)
{
  bool retval = false;
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;
  asyncio_tx_request_t *p_req = (asyncio_tx_request_t *) token;

  if (p_asyncio &&
      !p_asyncio->stream_stopped &&
      atomic_load(&p_asyncio->tx_task.init_state) == AsyncioInitialized &&
      p_req && size > 0 && size < p_asyncio->tx_task.max_tx_data_chunk_size) {
    p_req->size = size;
    p_req->fn = fn;
    p_req->p_private = p_private;
    retval = dispatch_async_f(p_asyncio->tx_task.worker, tx_f, p_asyncio, p_req);
  }
  return retval;
}

void asyncio_release_rx_buffer(asyncio_handle_t h_asyncio, asyncio_message_t message)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;

  if (p_asyncio && message) {
    pool_free(p_asyncio->rx_task.buffer_pool, message);
  }
}

static void asyncio_configure_tx_thread(asyncio_create_params_t *p_params,
                                        asyncio_interface_instance_t *p_asyncio)
{
  asyncio_tx_thread_context_t *pCtx = &p_asyncio->tx_task;

  CUTILS_ASSERTF(atomic_load(&pCtx->init_state) == AsyncioUninitialized, "Tx Thread should be uninitialized");
  strncpy(pCtx->thread_name, p_params->stream_name, ASYNCIO_MAX_THREAD_NAME - 3);
  strcat(pCtx->thread_name, "Tx");
  pCtx->max_tx_data_chunk_size = p_params->tx_max_chunk_size;
  pCtx->pool_create_params = p_params->tx_pool_create_params;
  pCtx->tx_write_timeout = p_params->tx_write_timeout;
  pCtx->tx_buffer_offset = p_params->tx_buffer_offset;
  pCtx->worker = p_params->tx_worker;
}

static void asyncio_configure_rx_thread(asyncio_create_params_t *p_params,
                                        asyncio_interface_instance_t *p_asyncio)
{
  asyncio_rx_thread_context_t *p_ctx = &p_asyncio->rx_task;

  CUTILS_ASSERTF(atomic_load(&p_ctx->init_state) == AsyncioUninitialized, "Rx Thread should be uninitialized");
  strncpy(p_ctx->thread_name, p_params->stream_name, ASYNCIO_MAX_THREAD_NAME - 3);
  strcat(p_ctx->thread_name, "Rx");
  p_ctx->worker = p_params->rx_worker;
  p_ctx->rx_pool_params = p_params->rx_pool_create_params;
  p_ctx->rx_callback = p_params->rx_callback;
  p_ctx->client_data = p_params->client_data;
}

static inline void trigger_start_completion(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_rx_thread_context_t *p_rx_ctx = &p_asyncio->rx_task;
  asyncio_tx_thread_context_t *p_tx_ctx = &p_asyncio->tx_task;

  if ((!p_rx_ctx->worker || atomic_load(&p_rx_ctx->init_state) == AsyncioInitialized) &&
      (!p_tx_ctx->worker || atomic_load(&p_tx_ctx->init_state) == AsyncioInitialized)) {
    atomic_store(&p_asyncio->init_state, AsyncioInitialized);
    event_flag_send(&p_asyncio->event_flag, STREAM_STARTED_FLAG);
  }
}

static void starter_f(void* arg1, void* arg2)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t*)arg1;
  bool is_rx = ((size_t)arg2 == 0) ? true : false;
  asyncio_tx_thread_context_t *p_tx_ctx = &p_asyncio->tx_task;
  asyncio_rx_thread_context_t *p_rx_ctx = &p_asyncio->rx_task;

  if(is_rx) {
    p_rx_ctx->buffer_pool = pool_create(&p_rx_ctx->rx_pool_params);
    CUTILS_ASSERTF(p_rx_ctx->buffer_pool, "Failed to allocate Rx Buffer Pool");
    atomic_store(&p_rx_ctx->init_state, AsyncioInitialized);
    STREAM_CLOGV("Starting Rx Worker Action");
    dispatch_async_f(p_rx_ctx->worker, rx_f, p_asyncio, 0);
  } else {
    STREAM_CLOGV("Tx Pool = %lu, %lu, %lu",
        p_tx_ctx->pool_create_params.num_of_elements,
        p_tx_ctx->pool_create_params.element_size_requested,
        p_tx_ctx->pool_create_params.total_element_size)
    p_tx_ctx->buffer_pool = pool_create(&p_tx_ctx->pool_create_params);
    CUTILS_ASSERTF(p_tx_ctx->buffer_pool, "Failed to allocate Tx Buffer Pool");
    atomic_store(&p_tx_ctx->init_state, AsyncioInitialized);
    STREAM_CLOGV("Tx Worker Ready for Transmission");
  }
  trigger_start_completion(p_asyncio);
}

static bool asyncio_start_tx_thread(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_tx_thread_context_t *p_ctx = &p_asyncio->tx_task;
  atomic_store(&p_ctx->init_state, AsyncioInitializing);
  dispatch_async_f(p_ctx->worker, starter_f, p_asyncio, (void*)1);
  return true;
}

static bool asyncio_start_rx_thread(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_rx_thread_context_t *p_ctx = &p_asyncio->rx_task;

  CUTILS_ASSERTF(p_ctx->worker, "Worker Must Be provided");
  CUTILS_ASSERTF(p_ctx->rx_callback, "An RxCallback that handles read out messages must be provided");
  atomic_store(&p_ctx->init_state, AsyncioInitializing);
  dispatch_async_f(p_ctx->worker, starter_f, p_asyncio, 0);
  return true;
}

static inline void trigger_stop_completion(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_rx_thread_context_t *p_rx_ctx = &p_asyncio->rx_task;
  asyncio_tx_thread_context_t *p_tx_ctx = &p_asyncio->tx_task;
  if (atomic_load(&p_rx_ctx->init_state) == AsyncioUninitialized &&
      atomic_load(&p_tx_ctx->init_state) == AsyncioUninitialized) {
    atomic_store(&p_asyncio->init_state, AsyncioUninitialized);
    event_flag_send(&p_asyncio->event_flag, STREAM_STOPPED_FLAG);
  }
}

static void finisher_f(void *arg1, void *arg2)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t*)arg1;
  asyncio_rx_thread_context_t *p_rx_ctx = &p_asyncio->rx_task;
  asyncio_tx_thread_context_t *p_tx_ctx = &p_asyncio->tx_task;
  int expected = AsyncioUninitializing;
  if( (size_t)arg2 == 0) {
    if (atomic_compare_exchange_strong(&p_rx_ctx->init_state, &expected, AsyncioUninitialized)) {
      pool_destroy(p_rx_ctx->buffer_pool);
      STREAM_SLOG("Stopped Rx");
    }
  } else {
    if (atomic_compare_exchange_strong(&p_tx_ctx->init_state, &expected, AsyncioUninitialized)) {
      pool_destroy(p_tx_ctx->buffer_pool);
      STREAM_SLOG("Stopped Tx");
    }
  }
  trigger_stop_completion(p_asyncio);
}

static void asyncio_stop_rx_thread(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_rx_thread_context_t *p_ctx = &p_asyncio->rx_task;
  int expected = AsyncioInitialized;
  //TODO: Make sure no more task are being pushed to the dispatch_queue
  if (atomic_compare_exchange_strong(&p_ctx->init_state, &expected, AsyncioUninitializing) ){
    dispatch_async_f(p_ctx->worker, finisher_f, p_asyncio, 0);
  } else {
    STREAM_SLOG("No Stop Rx Thread - %s", get_asyncio_init_state_e_string(atomic_load(&p_ctx->init_state)));
  }
  CUTILS_ASSERTF(expected != AsyncioInitializing, "Unexpected state %s", get_asyncio_init_state_e_string(expected));
}

static void asyncio_stop_tx_thread(asyncio_interface_instance_t *p_asyncio)
{
  asyncio_tx_thread_context_t *p_ctx = &p_asyncio->tx_task;
  int expected = AsyncioInitialized;
  if(atomic_compare_exchange_strong(&p_ctx->init_state, &expected, AsyncioUninitializing)) {
    dispatch_async_f(p_ctx->worker, finisher_f, p_asyncio, (void*)1);
  } else {
    STREAM_SLOG("No Stop Tx Thread - %s", get_asyncio_init_state_e_string(atomic_load(&p_ctx->init_state)));
  }
  CUTILS_ASSERTF(expected != AsyncioInitializing, "Unexpected state %s", get_asyncio_init_state_e_string(expected));
}

static void tx_f(void *arg1, void *arg2)
{
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) arg1;
  asyncio_tx_request_t *p_req = (asyncio_tx_request_t *) arg2;
  uint32_t timeout = p_asyncio->tx_task.tx_write_timeout;
  size_t bytes_written = 0;
  asyncio_tx_send_status_e status = StreamTxSendSuccess;
  CUTILS_ASSERTF(p_asyncio, "No Asyncio IO Handle!!!");

  if (atomic_load(&p_asyncio->init_state) == AsyncioInitialized) {
    if (!p_asyncio->is_in_error) {
      STREAM_CLOGV("Sending Data, Buf = %p",
                   asyncio_tx_request_get_buffer(p_req));
      bytes_written = p_asyncio->f_stream_interface_write(p_asyncio,
                                                          p_req->size,
                                                          asyncio_tx_request_get_buffer(p_req),
                                                          timeout);
      if (bytes_written != p_req->size) {
        STREAM_SLOG("%s(): bytes Written %u != req Size %u", __FUNCTION__, bytes_written, p_req->size);
        status = StreamTxSendMessageFail;
      }
    } else {
      STREAM_SLOG("Stream Interface is in error");
      status = StreamTxInterfaceInError;
    }

    if (p_req->fn) {
      p_req->fn((asyncio_tx_token_t) p_req, status, bytes_written, p_req->p_private);
    }
  }
  pool_free(p_asyncio->tx_task.buffer_pool, p_req);
}

static void rx_f(void* arg1, void* arg2)
{
  uint8_t *p_buf = 0;
  size_t bytes_read = 0;
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) arg1;
  asyncio_rx_thread_context_t *p_rx_ctx = &p_asyncio->rx_task;

  if (atomic_load(&p_rx_ctx->init_state) == AsyncioInitialized) {
    p_buf = pool_alloc(p_rx_ctx->buffer_pool);
    if (p_buf) {
      bytes_read = p_asyncio->f_stream_interface_read(p_asyncio, p_buf, 1000);
      if (bytes_read) {
        p_rx_ctx->rx_callback(p_asyncio, p_buf, bytes_read);
        dispatch_async_f(p_rx_ctx->worker, rx_f, p_asyncio, 0);
      } else {
        task_sleep(2);
        dispatch_async_f(p_rx_ctx->worker, rx_f, p_asyncio, 0);
      }
      pool_free(p_rx_ctx->buffer_pool, p_buf);
    } else {
      STREAM_SLOG("%s(): Couldn't allocate asyncio Rx Buffer", __FUNCTION__);
      task_sleep(10);
      dispatch_async_f(p_rx_ctx->worker, rx_f, p_asyncio, 0);
    }
  }
}

void *asyncio_get_private_data(asyncio_handle_t h_asyncio)
{
  void *retval = 0;
  asyncio_interface_instance_t *p_asyncio = (asyncio_interface_instance_t *) h_asyncio;

  if (p_asyncio) {
    retval = p_asyncio->rx_task.client_data;
  }
  return retval;
}
