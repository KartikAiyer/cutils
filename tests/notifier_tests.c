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

#include <embUnit/embUnit.h>
#include <cutils/notifier.h>
#include <cutils/task.h>
#include <malloc.h>
#include <stdlib.h>

/**
 * @struct test_notification_t
 * This is an example of a client notification. The test client exposes a callback defined by callback_f
 * This structure is derived from the notifier_block_t
 * @brief
 */
typedef struct _test_notification_t
{
  notifier_block_t base;
  void (*callback_f) (uint32_t category, uint32_t val);
  uint32_t posted_val;
} test_notification_t;

typedef struct _test1_data_t
{
  notifier_create_params_t create_params;
} test1_data_t;

/**
 * This creates the static storage needed to create a notifier that can have 256 categories and can have a total of
 * 256 callback registrations. The details of each callback are represented by test_notification_t
 */
NOTIFIER_STORE_DECL(notifier_test1, 256, 256, sizeof(test_notification_t));
NOTIFIER_STORE_DEF(notifier_test1);

static test1_data_t s_data;

static void counting_notification_post(notifier_t * notifier, uint32_t max_cat, uint32_t max_not);
static void setUp(void)
{
  srand(task_get_ticks());
}

static void tearDown(void)
{
  notifier_deinit(s_data.create_params.notifier);
}

static inline uint32_t get_count_of_posts_for_category(uint32_t category,
                                                       uint32_t max_posts,
                                                       test_notification_t ** posted_array)
{
  uint32_t retval = 0;

  for (uint32_t i = 0; i < max_posts; i++) {
    retval += (posted_array[i]->posted_val == category) ? 1 : 0;
  }
  return retval;
}

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

static void can_post_notification(void)
{
  NOTIFIER_STORE_CREATE_PARAMS_INIT(s_data.create_params,
                                    notifier_test1,
                                    test_notifier_callback,
                                    "test_notif",
                                    true);
  TEST_ASSERT(notifier_init(&s_data.create_params));
  TEST_ASSERT(s_data.create_params.notifier );
  counting_notification_post(s_data.create_params.notifier,
                             s_data.create_params.total_categories,
                             s_data.create_params.max_registrations);
}

static void counting_notification_post(notifier_t * notifier, uint32_t max_cat, uint32_t max_not)
{
  posted_counts_t *count_array = malloc(sizeof(posted_counts_t) * max_cat);
  uint32_t number_of_posts = rand() % max_not;
  test_notification_t **posted_array = malloc(sizeof(test_notification_t *) * number_of_posts);

  TEST_ASSERT(count_array);
  TEST_ASSERT(posted_array);
  memset(count_array, 0, sizeof(posted_counts_t) * max_cat);
  memset(posted_array, 0, sizeof(test_notification_t *) * number_of_posts);
  for (uint32_t i = 0; i < number_of_posts; i++) {
    uint32_t sample_input = rand() % max_cat;
    test_notification_t *notif = (test_notification_t *) notifier_allocate_notification_block(notifier);

    TEST_ASSERT(notif);
    posted_array[i] = notif;
    notif->posted_val = sample_input;
    count_array[sample_input].num_of_regs++;
    TEST_ASSERT(notifier_register_notification_block(notifier, sample_input, &notif->base));
  }
  for (uint32_t i = 0; i < number_of_posts; i++) {
    notifier_post_notification(notifier, posted_array[i]->posted_val, count_array);
  }

  for (uint32_t i = 0; i < max_cat; i++) {
    if (count_array[i].count && count_array[i].num_of_regs) {
      uint32_t count_in_posted = get_count_of_posts_for_category(i, number_of_posts, posted_array);

      if (count_in_posted != (count_array[i].count / count_array[i].num_of_regs)) {
        CLOG_FIELD(i, u);
        CLOG_FIELD(count_in_posted, u);
        CLOG_FIELD((count_array[i].count / count_array[i].num_of_regs), u);
        CLOG_FIELD(count_array[i].count, u);
        CLOG_FIELD(count_array[i].num_of_regs, i);
        CLOG_FIELD(number_of_posts, u);
        TEST_FAIL("Failed to tally up posts");
      }
    }
  }
  for (uint32_t i = 0; i < number_of_posts; i++) {
    notifier_deregister_notification_block(notifier, &posted_array[i]->base);
  }
  free(count_array);
  free(posted_array);
}

typedef struct
{
  notifier_create_params_t params;
} static_notif_test_data_t;
static static_notif_test_data_t s_test2 = { 0 };

NOTIFIER_STORE_DECL(test_notif_store, 10, 64, sizeof(test_notification_t));
NOTIFIER_STORE_DEF(test_notif_store);

static void setup2(void)
{

}

static void teardown2(void)
{
  notifier_deinit(s_test2.params.notifier);
}

static void static_notifier_store_can_post(void)
{
  NOTIFIER_STORE_CREATE_PARAMS_INIT(s_test2.params, test_notif_store,
                                                     test_notifier_callback, "TestNotif2", true);
  TEST_ASSERT_MESSAGE(notifier_init(&s_test2.params), "Could not create static notifier");
  counting_notification_post(s_test2.params.notifier,
                             GetArraySize(NOTIFIER_STORE(test_notif_store).notifierArray),
                             s_test2.params.notifier->p_pool->num_of_elements);
}

TestRef notifier_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixture) {
    new_TestFixture("Can post notifications", can_post_notification)
  };
  EMB_UNIT_TESTCALLER(notifier_tests, "Notifier Tests", setUp, tearDown, fixture);
  return (TestRef) & notifier_tests;
}

TestRef notifier_static_store_get_tests(void)
{
  EMB_UNIT_TESTFIXTURES(fixture) {
    new_TestFixture("Can post notifications to Notifer with static store", static_notifier_store_can_post)
  };
  EMB_UNIT_TESTCALLER(notifier_tests, "Notifier Static Store Tests", setup2, teardown2, fixture);
  return (TestRef) & notifier_tests;
}

int main()
{
  TestRunner_start();
  {
    TestRunner_runTest(notifier_get_tests());
    TestRunner_runTest(notifier_static_store_get_tests());
  }
  TestRunner_end();
  return 0;
}
