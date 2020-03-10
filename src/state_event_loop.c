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

#include <cutils/state_event_loop.h>
#include <cutils/logger.h>


#define SEL_CLOG(str, ...) console_log(&event_loop->log, NULL, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

//TODO: Reenbale this
#define SEL_SLOG(str, ...)
//system_log(&event_loop->log, NULL, "%s(%u):" str, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define EVENT_SLOG(p_event, str, ...)
//system_log(&event_loop->log, p_event,  "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define CLOGV(str, ...) CLOG_IF(event_loop->log.log_level > 0, str, ##__VA_ARGS__)
#define SLOGV(str, ...) SLOG_IF(event_loop->log.log_level > 0, str, ##__VA_ARGS__)
#define DLOGV(str, ...) DLOG_IF(event_loop->log.log_level > 0, str, ##__VA_ARGS__)

//Forward Declaration
static uint32_t state_event_loop_logger_prefix(logger_t* logger, void* client_data, char* str, uint32_t string_size);

state_event_loop_t *state_event_loop_init(state_event_loop_create_params_t * create_params)
{
  dispatch_queue_create_params_t dispatch_create_params = create_params->queue_params;

  CUTILS_ASSERTF(create_params->p_event_loop, "Invalid Event Loop Pointer provided");
  CUTILS_ASSERTF(create_params->p_sm, "No State Machines provided");
  CUTILS_ASSERTF(create_params->event_data_size > sizeof(event_t), "Invalid size (%lu) of event data, expected %lu",
               create_params->event_data_size, sizeof(event_t));

  state_event_loop_t *event_loop = create_params->p_event_loop;

  // Create Exec Queue
  event_loop->event_data_size = create_params->event_data_size;
  event_loop->p_exec_queue = dispatch_queue_create(&dispatch_create_params);
  event_loop->log.isEnabled = true;
  event_loop->name = create_params->name;
  event_loop->log.fnPrefix = state_event_loop_logger_prefix;
  event_loop->log.log_level = 0;
  CUTILS_ASSERTF( event_loop->p_exec_queue, "Couldn't create exec queue");
  if (event_loop->p_exec_queue) {
    // Create State Machine
    event_loop->num_machines = create_params->num_machines;
    event_loop->p_sm = create_params->p_sm;
    for( uint32_t i = 0; i < event_loop->num_machines; i++) {
      CLOGV("Will Create SM %s", create_params->p_sm_create_params[i].p_name);
      state_machine_create(&create_params->p_sm_create_params[i]);
    }

    //Create Notifier that will be used to hold listeners of events posted.
    CUTILS_ASSERTF(notifier_init(&create_params->notifier_create_params), "Could not initialize notifier");
    event_loop->notifier = create_params->notifier_create_params.notifier;

    // Create Block pool of all events that will be used as posted data
    event_loop->p_pool_of_events = pool_create(&create_params->event_pool_create_params);
    CUTILS_ASSERTF(event_loop->p_pool_of_events, "Couldn't create Block Pool for $s", create_params->name);

    if ((event_loop->p_pool_of_events->element_size != create_params->event_data_size)) {
      SEL_CLOG("%s(): pool block size %u != event data size %u",
           __FUNCTION__, event_loop->p_pool_of_events->element_size, create_params->event_data_size);
      pool_destroy(event_loop->p_pool_of_events);
      notifier_deinit(event_loop->notifier);
      event_loop->p_pool_of_events = 0;
      event_loop->notifier = 0;
      return NULL;
    }
    if (event_loop->p_pool_of_events->num_of_elements != create_params->queue_params.queue_params.size) {
      SEL_CLOG("%s(): blocks available %u != queue_size %u", __FUNCTION__,
           event_loop->p_pool_of_events->num_of_elements, create_params->queue_params.queue_params.size);
      pool_destroy(event_loop->p_pool_of_events);
      notifier_deinit(event_loop->notifier);
      event_loop->p_pool_of_events = 0;
      event_loop->notifier = 0;
      return NULL;
    }
  }
  return event_loop;
}

void state_event_loop_deinit(state_event_loop_t * event_loop)
{
  CUTILS_ASSERT(event_loop);
  SEL_SLOG("Destroying");
  dispatch_queue_destroy(event_loop->p_exec_queue);
  state_event_loop_stop(event_loop);
  notifier_deinit(event_loop->notifier);
  pool_destroy(event_loop->p_pool_of_events);
  event_loop->p_pool_of_events = 0;
  SEL_SLOG("Destroyed");
}

void state_event_loop_add_state(state_event_loop_t * event_loop, state_t * state, uint32_t state_mac_id)
{
  if (event_loop) {
    state_machine_register_state(&event_loop->p_sm[state_mac_id], state);
  }
}

void state_event_loop_start(state_event_loop_t *event_loop)
{
  if (event_loop) {
    for(uint32_t i = 0; i < event_loop->num_machines; i++) {
      state_machine_start(&event_loop->p_sm[i]);
    }
  }
}

void state_event_loop_stop(state_event_loop_t * event_loop)
{
  if (event_loop) {
    for(uint32_t i = 0; i < event_loop->num_machines; i++) {
     state_machine_stop(&event_loop->p_sm[i]);
    }
    // TODO: Other things maybe... like stop allowing notifications to post.
  }
}

bool state_event_loop_install_event_pre_proc(state_event_loop_t *event_loop,
                                             void (*fn)(event_t *, void *),
                                             void *client_data)
{
  bool retval = false;
  if(!event_loop->p_sm->state_machine_started) {
    event_loop->event_pre_proc_f = fn;
    event_loop->client_data = client_data;
  }
  return retval;
}

void state_event_loop_exec_queue_f(void *arg1, void *arg2)
{
  event_t *event = (event_t *) arg1;
  state_event_loop_t *event_loop = event->event_loop;

  // pass the event to the state machine
  uint32_t time_taken = task_get_ticks();

  if (event_loop->event_pre_proc_f) event_loop->event_pre_proc_f(event, event_loop->client_data);

  for (uint32_t i = 0; i < event_loop->num_machines; i++) {
    state_machine_handle_event(&event_loop->p_sm[i], event);
    state_machine_transition(&event_loop->p_sm[i]);
  }
  notifier_post_notification(event_loop->notifier, event->event_id, event);
  time_taken = task_get_ticks() - time_taken;
  EVENT_SLOG(event, "Processed: %u ms", time_taken);
  state_event_loop_release_event(event_loop, event);
}

state_event_registration_t *state_event_loop_allocate_registration(state_event_loop_t * event_loop)
{
  state_event_registration_t *retval = 0;

  if (event_loop) {
    retval = (state_event_registration_t *) notifier_allocate_notification_block(event_loop->notifier);
  }
  return retval;
}

bool state_event_loop_register_notification(state_event_loop_t * event_loop,
                                            uint32_t event_type, state_event_registration_t * p_reg)
{
  bool retval = false;

  if (event_loop &&
      p_reg && notifier_register_notification_block(event_loop->notifier, event_type, &p_reg->base)) {
    p_reg->p_event_loop = event_loop;
    retval = true;
  }
  return retval;
}

void state_event_loop_deregister_notification(state_event_loop_t * event_loop,
                                              state_event_registration_t * p_reg)
{
  if (event_loop && p_reg) {
    notifier_deregister_notification_block(event_loop->notifier, &p_reg->base);
  }
}

uint32_t state_event_loop_get_current_state_id(state_event_loop_t* event_loop, uint32_t state_mac_index)
{
  CUTILS_ASSERTF(event_loop &&
               state_mac_index < event_loop->num_machines &&
               event_loop->p_sm[state_mac_index].current_state, "Invalid Inputs");
  return event_loop->p_sm[state_mac_index].current_state->stateId;
}

state_t* state_event_loop_get_current_state(state_event_loop_t* event_loop, uint32_t state_mac_index)
{
  CUTILS_ASSERTF(event_loop &&
               state_mac_index < event_loop->num_machines, "Invalid Inputs");
  return event_loop->p_sm[state_mac_index].current_state;
}

uint32_t state_event_loop_state_prefix(logger_t* p_logger, void* p_private, char* p_string, uint32_t string_size)
{
  uint32_t retval = 0;
  state_t* state = (state_t*)((void*)p_logger - offsetof(state_t, log));
  event_t* p_event = (event_t*)p_private;

  if(string_size > strlen(state->state_name)) {
    INSERT_TO_STRING(p_string, retval, "%s: ", state->state_name);
  }
  if( p_event && p_event->to_string) {
    //An Event has been supplied.
    retval += p_event->to_string(p_event, p_string + retval, string_size - retval);
  }

  return retval;
}

static uint32_t state_event_loop_logger_prefix(logger_t* logger, void* client_data, char* str, uint32_t string_size)
{
  uint32_t retval = 0;
  state_event_loop_t *p_event_loop = (state_event_loop_t*)((void*)logger - offsetof(state_event_loop_t, log));
  INSERT_TO_STRING(str, retval, "%s", p_event_loop->name);
  for(uint32_t i = 0; i < p_event_loop->num_machines; i++) {
    if(p_event_loop->p_sm && p_event_loop->p_sm[i].current_state) {
      INSERT_TO_STRING(str,  retval,":%s", p_event_loop->p_sm[i].current_state->state_name);
    }
  }
  if(client_data) {
    //Event has been provided
    event_t* event = (event_t*)client_data;
    if(event->to_string) {
      INSERT_TO_STRING(str, retval, ":");
      retval += event->to_string(event, &str[retval], string_size - retval);
    }
  }
  return retval;
}

