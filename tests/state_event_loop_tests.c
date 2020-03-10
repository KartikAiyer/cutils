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
#include <cutils/state_event_loop.h>
#include <stdlib.h>

#define TEST_MAX_STATES                 ( 20 )
#define TEST_QUEUE_SIZE                 ( 64 )
#define TEST_MAX_REGISTRATIONS          ( 64 )
#define TEST_FLAG_PRIVATE_DATA_VALID    (1U << 0U)
#define TEST_FLAG_PRIVATE_DATA_INVALID  (1U << 1U)

#define  PRINT_FIELD( x, fmt )    CLOG( #x " = %" #fmt, x )

typedef enum
{
  TRANSITION_EVENT,
  NO_TRANSITION_EVENT,
  TEST_MAX_EVENT_ID
} tests_event_e;

typedef struct
{
  event_t base;
  uint32_t next_state;
} test_event_t;

typedef struct
{
  state_t base;
  uint32_t exit_state;
} test_state_t;

/**
 * Define your own callback for notifications
 */
typedef struct
{
  notifier_block_t base;
} test_registration_t;

typedef struct
{
  state_t states[TEST_MAX_STATES];
  state_event_loop_create_params_t create_params;
  state_event_loop_t *p_loop;
  event_flag_t flags;
  uint32_t init_count;
} sel_test_data_t;

static sel_test_data_t s_sel_data;

STATE_EVENT_LOOP_STORE_DECL(test_evl,
                            1,
                            TEST_QUEUE_SIZE,
                            TEST_MAX_REGISTRATIONS,
                            TEST_MAX_EVENT_ID,
                            16384,
                            sizeof(test_registration_t),
                            sizeof(test_event_t));

STATE_EVENT_LOOP_STORE_DEF(test_evl);

typedef struct
{
  uint32_t count;
  uint32_t num_of_regs;
} posted_counts_t;

static void test_notifier_callback(notifier_block_t * block, uint32_t category, void *notif_data)
{
  posted_counts_t *count_array = (posted_counts_t *) notif_data;

  count_array[category].count++;
}

static void setup(void)
{
  char* sm_names[] = { "test_event_loop" };
  uint32_t start_state[] = { 0 };
  memset(&s_sel_data, 0, sizeof(s_sel_data));
  STATE_EVENT_LOOP_CREATE_PARAMS_INIT(s_sel_data.create_params,
                                      test_evl,
                                      sm_names[0],
                                      CUTILS_TASK_PRIORITY_MEDIUM,
                                      test_notifier_callback,
                                      sm_names,
                                      start_state,
                                      &s_sel_data);
  event_flag_new(&s_sel_data.flags);
}

static void teardown(void)
{
  event_flag_free(&s_sel_data.flags);
}

static void test_state_init(state_machine_t * sm, state_t * state)
{
  test_state_t *test_state = (test_state_t *) state;
  sel_test_data_t *p_loop = (sel_test_data_t *) state_machine_get_private_data(sm);
  if (p_loop == &s_sel_data) {
    event_flag_send(&s_sel_data.flags, TEST_FLAG_PRIVATE_DATA_VALID);
  } else {
    event_flag_send(&s_sel_data.flags, TEST_FLAG_PRIVATE_DATA_INVALID);
  }
  test_state->exit_state = (test_state->base.stateId + 1) % TEST_MAX_STATES;
  p_loop->init_count++;
}

static void test_state_enter(state_machine_t * sm, state_t * state)
{
}

static void test_state_exit(state_machine_t * sm, state_t * state)
{
}

static bool test_state_valid_event(state_machine_t * sm, state_t * state, void *evt)
{
  test_event_t *event = (test_event_t *) evt;

  if (event->base.event_id < TEST_MAX_EVENT_ID)
    return true;
  else
    return false;
}

static uint32_t test_state_handle_event(state_machine_t * sm, state_t * state, void *evt)
{
  uint32_t next_state = state->stateId;
  test_state_t *test_state = (test_state_t *) state;
  test_event_t *event = (test_event_t *) evt;

  switch (event->base.event_id) {
  case TRANSITION_EVENT:
    next_state = test_state->exit_state;
    break;
  case NO_TRANSITION_EVENT:
    break;
  default:
    break;
  }
  return next_state;
}

static void can_post_to_state_machine(void)
{
  uint32_t total_posts = 80;

  if (s_sel_data.create_params.event_data_size != sizeof(test_event_t)) {
    CLOG("Event Data size no match: event_data_size = %u, sizeof(test_event_t) = %u",
         s_sel_data.create_params.event_data_size, sizeof(test_event_t));
    PRINT_FIELD(s_sel_data.create_params.event_data_size, u);
    PRINT_FIELD(s_sel_data.create_params.queue_params.queue_params.size, u);
    PRINT_FIELD(s_sel_data.create_params.queue_params.task_params.stack_size, u);
    PRINT_FIELD(s_sel_data.create_params.notifier_create_params.max_registrations, u);
    TEST_ASSERT(s_sel_data.create_params.event_data_size == sizeof(test_event_t));
  }
  s_sel_data.p_loop = state_event_loop_init(&s_sel_data.create_params);
  TEST_ASSERT(s_sel_data.p_loop);
  for (uint32_t i = 0; i < TEST_MAX_STATES; i++) {
    state_t *state = (state_t *) & s_sel_data.states[i];

    state->stateId = i;
    state->state_name = "Test State";
    state->f_init = test_state_init;
    state->f_enter = test_state_enter;
    state->f_exit = test_state_exit;
    state->f_handle_event = test_state_handle_event;
    state->f_valid_event = test_state_valid_event;
    state_event_loop_add_state(s_sel_data.p_loop, state, 0);
  }
  state_event_loop_start(s_sel_data.p_loop);
  TEST_ASSERT_EQUAL_INT(1, s_sel_data.init_count);
  uint32_t transition_count = 1;
  for (uint32_t i = 0; i < total_posts; i++) {
    test_event_t event = {.base.event_id = (uint32_t) rand() % TEST_MAX_EVENT_ID };
    uint32_t last_state_id = state_event_loop_get_current_state_id(s_sel_data.p_loop, 0);

    state_event_loop_post(s_sel_data.p_loop, event.base.event_id, &event.base);
    task_sleep(1);
    uint32_t current_state_id = state_event_loop_get_current_state_id(s_sel_data.p_loop, 0);

    if (event.base.event_id == TRANSITION_EVENT) {
      uint32_t expected_new_state = (last_state_id + 1) % TEST_MAX_STATES;
      transition_count++;
      if(transition_count <= TEST_MAX_STATES) {
        TEST_ASSERT_EQUAL_INT(transition_count, s_sel_data.init_count);
      } else {
        TEST_ASSERT_EQUAL_INT(TEST_MAX_STATES, s_sel_data.init_count);
      }
      TEST_ASSERT_EQUAL_INT(expected_new_state, current_state_id);
    } else {
      TEST_ASSERT_EQUAL_INT(last_state_id, current_state_id);
    }
  }
  state_event_loop_stop(s_sel_data.p_loop);
  state_event_loop_deinit(s_sel_data.p_loop);
  uint32_t actual_flags = 0;
  TEST_ASSERT(event_flag_wait(&s_sel_data.flags,
                              TEST_FLAG_PRIVATE_DATA_INVALID | TEST_FLAG_PRIVATE_DATA_VALID,
                              WAIT_OR_CLEAR,
                              &actual_flags,
                              NO_SLEEP));
  TEST_ASSERT(!(actual_flags & TEST_FLAG_PRIVATE_DATA_INVALID));
  TEST_ASSERT(actual_flags & TEST_FLAG_PRIVATE_DATA_VALID);
  TEST_ASSERT_EQUAL_INT(TEST_MAX_STATES, s_sel_data.init_count);
}

TestRef state_event_loop_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixtures) {
    new_TestFixture("Should post to state machine", can_post_to_state_machine)
  };
  EMB_UNIT_TESTCALLER(sel_tests, "State event loop tests", setup, teardown, fixtures);
  return (TestRef) & sel_tests;
}

int main()
{
  TestRunner_start();
  {
    TestRunner_runTest(state_event_loop_get_tests());
  }
  TestRunner_end();
  return 0;
}
