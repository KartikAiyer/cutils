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

#include <embUnit/embUnit.h>
#include <cutils/asyncio.h>
#include <cutils/accumulator.h>
#include <inttypes.h>
#include <stdlib.h>

#define MESSAGE_BUF_SIZE        ( 64 )
ASYNCIO_STORE_DECL(testio, 16, 16, MESSAGE_BUF_SIZE, MESSAGE_BUF_SIZE, 16);
ASYNCIO_STORE_DEF(testio);

ASYNCIO_STORE_DECL(tx_only_test, ASYNCIO_SERVICE_UNUSED, 2, ASYNCIO_SERVICE_UNUSED, MESSAGE_BUF_SIZE, 16);
ASYNCIO_STORE_DEF(tx_only_test);

ACCUMULATOR_STORE_DECL(test_acc, 1024);
ACCUMULATOR_STORE_DEF(test_acc);

DISPATCH_QUEUE_STORE_DECL(asyncio_rx_q, 4, 64*1024);
DISPATCH_QUEUE_STORE_DEF(asyncio_rx_q);

DISPATCH_QUEUE_STORE_DECL(asyncio_tx_q, 4, 64*1024);
DISPATCH_QUEUE_STORE_DEF(asyncio_tx_q);


#define BUF_AVAIL_TO_READ     ( 1 << 0 )
#define TEST_COMPLETE         ( 1 << 1 )

#define TEST_STATUS_E(XX)\
  XX( TestPassed, = 0)\
  XX( TestContinuing, )\
  XX( TestFailedUnexpectedBufferSize, )\
  XX( TestFailedCrcMismatch, )\
  XX( TestFaileToAllocateToken, )\
  XX( TestFailedBadBufferInToken, )\
  XX( TestFailedSendData, )

DECLARE_ENUM(testio_test_status_e, TEST_STATUS_E)

typedef struct
{
  uint32_t signature;
  event_flag_t evt;
  accumulator_h acc;
  dispatch_queue_t* tx_queue;
  dispatch_queue_t* rx_queue;
  uint8_t tempBuf[MESSAGE_BUF_SIZE];
  testio_test_status_e status;
  char err_str[256];
  uint32_t total_num_of_test_packets;
  uint32_t current_proc_packets;
} testio_data_t;

static testio_data_t s_data = {.signature = 0xDEADBEEF, .total_num_of_test_packets = 30};

static uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

static void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  if(!*table)
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for(size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

static inline void generate_random_payload(uint32_t* buf)
{
  if (buf) {
      for (uint32_t i = 0; i < (MESSAGE_BUF_SIZE - sizeof(uint32_t)) / sizeof(uint32_t); i++) {
        buf[i] = (uint32_t) rand();
      }
  }
}

static size_t validate_accumulator(testio_data_t *p_data)
{
  size_t retval = accumulator_bytes_contained(p_data->acc);
  if (retval <= MESSAGE_BUF_SIZE) {
    uint32_t crc = 0;

    accumulator_extract(p_data->acc, (uint8_t *) &crc, sizeof(crc));
    accumulator_extract(p_data->acc, p_data->tempBuf, (uint32_t) (retval - sizeof(crc)));

    uint32_t calculated_crc = 0;
    crc32(p_data->tempBuf, (uint32_t) (retval - sizeof(crc)), &calculated_crc);

    if (calculated_crc != crc) {
      p_data->status = TestFailedCrcMismatch;
      snprintf(p_data->err_str, sizeof(p_data->err_str),
               "Crc Mismatch - Expected = 0x%"PRIx32", Got = 0x%"PRIx32, crc, calculated_crc);
    } else {
      p_data->status = TestContinuing;
    }
  } else {
    p_data->status = TestFailedUnexpectedBufferSize;
    snprintf(p_data->err_str, sizeof(p_data->err_str), "Recd: %"PRIu64" bytes, expected %"PRIu32" bytes",
             retval, MESSAGE_BUF_SIZE);
  }
  return retval;
}

static size_t testio_read_f(asyncio_handle_t handle, uint8_t *p_rx_data_buf, uint32_t timeout)
{
  size_t retval = 0;
  testio_data_t *p_data = (testio_data_t *) asyncio_get_private_data(handle);
  uint32_t act_flags = 0;

  if (event_flag_wait(&p_data->evt, BUF_AVAIL_TO_READ, WAIT_OR_CLEAR, &act_flags, timeout)) {
    retval = validate_accumulator(p_data);
  }

  return retval;
}

static size_t testio_write_f(asyncio_handle_t handle,
                             size_t tx_data_size,
                             uint8_t *p_tx_data_buf,
                             uint32_t timeout)
{
  size_t retval = 0;
  testio_data_t *p_data = (testio_data_t *) asyncio_get_private_data(handle);

  if (tx_data_size <= MESSAGE_BUF_SIZE - sizeof(uint32_t)) {
    uint32_t crc = 0;
    crc32(p_tx_data_buf, tx_data_size, &crc);

    accumulator_insert(p_data->acc, (uint8_t *) &crc, sizeof(crc));
    accumulator_insert(p_data->acc, p_tx_data_buf, tx_data_size);
    event_flag_send(&p_data->evt, BUF_AVAIL_TO_READ);
    retval = tx_data_size;
  } else {
    p_data->status = TestFailedUnexpectedBufferSize;
    snprintf(p_data->err_str, sizeof(p_data->err_str), "Trying to send %"PRIu64" bytes when %"PRIu32" is max",
             tx_data_size, MESSAGE_BUF_SIZE);
  }

  return retval;
}

static size_t testio_tx_only_write_f(asyncio_handle_t handle,
                                     size_t tx_data_size,
                                     uint8_t *p_tx_data_buf,
                                     uint32_t timeout)
{
  size_t retval = 0;
  //Just going to validate the data.
  testio_data_t *p_data = (testio_data_t*) asyncio_get_private_data(handle);
  if (tx_data_size <= MESSAGE_BUF_SIZE - sizeof(uint32_t)) {
    uint32_t crc = 0;
    crc32(p_tx_data_buf, tx_data_size, &crc);

    accumulator_insert(p_data->acc, (uint8_t *) &crc, sizeof(crc));
    accumulator_insert(p_data->acc, p_tx_data_buf, tx_data_size);
    validate_accumulator(p_data);
    retval = tx_data_size;
  } else {
    p_data->status = TestFailedUnexpectedBufferSize;
    snprintf(p_data->err_str, sizeof(p_data->err_str), "Trying to send %"PRIu64" bytes when %"PRIu32" is max",
             tx_data_size, MESSAGE_BUF_SIZE);
  }
  return retval;
}

static bool send_random_payload(testio_data_t *p_data, asyncio_handle_t h_asyncio, asyncio_tx_notification_f fn)
{
  asyncio_tx_token_t token = asyncio_allocate_tx_token(h_asyncio);
  bool retval = false;
  CUTILS_ASSERT(p_data);
  if (token) {
    uint32_t *buf = (uint32_t *) asyncio_tx_token_get_data_buffer(token);
    if (buf) {
      generate_random_payload(buf);
      retval = asyncio_send_buffer(h_asyncio, token, MESSAGE_BUF_SIZE - sizeof(uint32_t), fn, NULL);
    } else {
      p_data->status = TestFailedBadBufferInToken;
      snprintf(p_data->err_str, sizeof(p_data->err_str), "No Send Buffer in Tx Token");
    }
  } else {
    p_data->status = TestFaileToAllocateToken;
    snprintf(p_data->err_str, sizeof(p_data->err_str), "Failed to allocate Tx Token");
  }
  return retval;
}

static void testio_rx_f(asyncio_handle_t h_asyncio, asyncio_message_t message, size_t size)
{

  testio_data_t *p_data = (testio_data_t *) asyncio_get_private_data(h_asyncio);

  CUTILS_ASSERTF(p_data, "Private data not supplied in Asyncio RX Callback");
  if (p_data->status == TestContinuing) {
    p_data->current_proc_packets++;
    if (p_data->current_proc_packets < p_data->total_num_of_test_packets) {
      // Send Random Payload
      if (!send_random_payload(p_data, h_asyncio, NULL)) {
        p_data->status = TestFailedSendData;
        snprintf(p_data->err_str, sizeof(p_data->err_str), "Failed to send Data chunk: %"PRIu32" / %"PRIu32,
                 p_data->current_proc_packets, p_data->total_num_of_test_packets);
        event_flag_send(&p_data->evt, TEST_COMPLETE);
      }
    } else {
      p_data->status = TestPassed;
      event_flag_send(&p_data->evt, TEST_COMPLETE);
    }
  } else {
    event_flag_send(&p_data->evt, TEST_COMPLETE);
  }
}

static void asyncio_loopback_test(void)
{
  asyncio_create_params_t params;
  TEST_ASSERT(s_data.rx_queue && s_data.tx_queue);
  ASYNCIO_CREATE_PARAMS_INIT(params,
                             testio,
                             "testio",
                             testio_read_f,
                             testio_write_f,
                             testio_rx_f,
                             s_data.rx_queue,
                             s_data.tx_queue,
                             &s_data);

  asyncio_handle_t h_asyncio = asyncio_create_instance(&params);

  TEST_ASSERT(h_asyncio);
  TEST_ASSERT(asyncio_start(h_asyncio));
  // Start the test
  if (!send_random_payload(&s_data, h_asyncio, NULL)) {
    TEST_FAIL("Failed to send payload over asyncio");
  } else {
    // Wait for the test to complete
    uint32_t act_flags = 0;

    if (!event_flag_wait(&s_data.evt,
                         TEST_COMPLETE,
                         WAIT_OR_CLEAR,
                         &act_flags,
                         WAIT_FOREVER)) {
      TEST_FAIL("Failed to wait for asyncio test to complete");
    } else {
      TEST_ASSERT_EQUAL_INT(TestPassed, s_data.status);
    }
  }
  asyncio_destroy_instance(h_asyncio);
}

static void asyncio_tx_only_notification_f(asyncio_tx_token_t token,
                                           asyncio_tx_send_status_e sendStatus,
                                           uint32_t sendSize,
                                           void *pPrivate)
{
  if (s_data.status == TestContinuing ) {
    s_data.current_proc_packets++;
    event_flag_send(&s_data.evt, BUF_AVAIL_TO_READ);
    if (s_data.current_proc_packets == s_data.total_num_of_test_packets) {
      s_data.status = TestPassed;
      event_flag_send(&s_data.evt, TEST_COMPLETE);
    }
  } else {
    event_flag_send(&s_data.evt, TEST_COMPLETE);
  }
}

static void asyncio_tx_only_test(void)
{
  uint32_t act_flags = 0;
  asyncio_create_params_t params;
  ASYNCIO_CREATE_PARAMS_INIT(params,
                             tx_only_test,
                             "tx_only_test",
                             ASYNCIO_SERVICE_UNUSED,
                             testio_tx_only_write_f,
                             ASYNCIO_SERVICE_UNUSED,
                             ASYNCIO_SERVICE_UNUSED,
                             s_data.tx_queue,
                             &s_data);

  asyncio_handle_t h_asyncio = asyncio_create_instance(&params);

  TEST_ASSERT(s_data.rx_queue && s_data.tx_queue);
  TEST_ASSERT(h_asyncio);
  TEST_ASSERT(asyncio_start(h_asyncio));
  for (uint32_t i = 0; i < s_data.total_num_of_test_packets; i++) {
    if (!send_random_payload(&s_data, h_asyncio, asyncio_tx_only_notification_f)) {
      TEST_FAIL("Failed to send payload over asyncio");
    }
    TEST_ASSERT(event_flag_wait(&s_data.evt, BUF_AVAIL_TO_READ, WAIT_OR_CLEAR, &act_flags, 1000));
  }
  act_flags = 0;
  if (!event_flag_wait(&s_data.evt,
                       TEST_COMPLETE,
                       WAIT_OR_CLEAR,
                       &act_flags,
                       WAIT_FOREVER)) {
    TEST_FAIL("Failed to wait for asyncio test to complete");
  } else {
    TEST_ASSERT_EQUAL_INT(TestPassed, s_data.status);
  }
  asyncio_destroy_instance(h_asyncio);
}

static void setup(void)
{
  accumulator_create_params_t acc_params;

  event_flag_new(&s_data.evt);
  event_flag_clear(&s_data.evt, 0xFFFFFFFF);

  ACCUMULATOR_CREATE_PARAMS_INIT(acc_params, test_acc);
  s_data.acc = accumulator_create(&acc_params);
  s_data.current_proc_packets = 0;
  s_data.total_num_of_test_packets = 30;

  dispatch_queue_create_params_t params = { 0 };
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, asyncio_tx_q, "asyncio_test_tx", CUTILS_TASK_PRIORITY_MEDIUM);
  s_data.tx_queue = dispatch_queue_create(&params);

  DISPATCH_QUEUE_CREATE_PARAMS_INIT(params, asyncio_rx_q, "asyncio_test_rx", CUTILS_TASK_PRIORITY_MID_LO);
  s_data.rx_queue = dispatch_queue_create(&params);
}

static void teardown(void)
{
  event_flag_free(&s_data.evt);
  accumulator_clear(s_data.acc);
  dispatch_queue_destroy(s_data.tx_queue);
  dispatch_queue_destroy(s_data.rx_queue);
  s_data.tx_queue = s_data.rx_queue = 0;
}

TestRef asyncio_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixtures) {
      new_TestFixture("Test Asyncio Loopback", asyncio_loopback_test),
      new_TestFixture("Test Asyncio Tx Only", asyncio_tx_only_test)
  };
  EMB_UNIT_TESTCALLER(asyncio_basic_tests, "Asyncio interface Tests", setup, teardown, fixtures);
  return (TestRef) &asyncio_basic_tests;
}
int main()
{
  TestRunner_start();
  {
    TestRunner_runTest(asyncio_get_tests());
  }
  TestRunner_end();
  return 0;
}
