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

#pragma once

#include <cutils/state_machine.h>
#include <cutils/notifier.h>
#include <cutils/pool.h>
#include <cutils/dispatch_queue.h>

typedef struct _event_t event_t;
/**
 * @struct state_event_loop_t
 * @brief: State Event loop is an abstraction that wraps a dispatch_queue_t,
 * a list of state machines and an event handler. Clients will use this to implement
 * singleton state machine/event handler objects. A state event loop transitions its state
 * machines using events and process all events within a single dispatch_queue. When an event is
 * posted, the current state in every state machine will be serially provided the event to process.
 * The result of processing an event in a state machine is the new state that it will transition to.
 * After all state machines have transitioned, the event is then dispatched to external listeners.
 *
 * External listeners can provide their own dispatch_queue to process the event. Implementaions can
 * choose to dumb this down and process all events in the state event loop dispatch queue.
 *
 * The state event loop also offers an api to maintain the listener registrations.
 *
 * Clients will have to implement the set of events that they wish to run through the system. An easy
 * way to do this is to define and enum type and an associated data structure that holds a union of
 * all event data types... ie.
 *
 * enum event_e { EVENT_A, EVENT_B, EVENT_C };
 * struct client_event {
 *   event_t base;
 *   event_e event_id;
 *   union {
 *      // event specific data types
 *   }event_data
 * };
 *
 */
typedef struct _state_event_loop_t
{
  state_machine_t* p_sm;
  uint32_t num_machines;
  dispatch_queue_t *p_exec_queue;
  pool_t * p_pool_of_events;
  notifier_t* notifier;
  uint32_t event_data_size;
  logger_t log;
  char* name;
  /**
   * @brief This is an optional callback that needs to be installed prior to starting the
   * state_event_loop. This will call a pre processor before any of the states event handling
   * and external listeners are notified about the event. This can be used by the state_event_loop
   * client to modify global data within its scope prior to others processing events.
   * @param event
   * @param client_data - optional client data typically a pointer to client context.
   */
  void (*event_pre_proc_f)(event_t* event, void* client_data);
  /**
   * @brief Optional client context.
   */
  void* client_data;
} state_event_loop_t;

/**
 * @struct state_event_registration_t
 *
 * @brief In order to allow implementations to specify their own listener callback signatures, implementation will
 * have to implement a function which defines the specifics of calling the implementation defined callbacks.
 * This is done by implementers providing a derived type to state_event_registration_t and implementing a notifier function
 * of type `notifier_f` ie void (*notifier_f) (notifier_block_t * pBlock, uint32_t category, void *pNotificationData);
 *
 * Whenever an event is being posted to a listener, the implenting notifier_f is called with a `notifier_block_t` which
 * is a base of `state_event_registration_t` which should be the base of the implmenting registration type. Thus the
 * implementing `notifier_f` can expect its registration which should contain all the mechanics to call the listener
 * functions.
 */

typedef struct _state_event_registration_t
{
  notifier_block_t base;
  state_event_loop_t *p_event_loop;
} state_event_registration_t;

typedef struct _event_t
{
  uint32_t event_id;
  state_event_loop_t *event_loop;
  uint32_t (*to_string)(struct _event_t* event, char* p_string, uint32_t string_size);
} event_t;

typedef struct
{
  state_event_loop_t *p_event_loop;
  notifier_create_params_t notifier_create_params;
  dispatch_queue_create_params_t queue_params;
  pool_create_params_t event_pool_create_params;
  state_machine_create_params_t* p_sm_create_params;
  state_machine_t* p_sm;
  uint32_t num_machines;
  char *name;
  uint32_t exec_queue_priority;
  uint32_t event_data_size;
} state_event_loop_create_params_t;

#define STATE_EVENT_LOOP_STORE( name )      event_loop_store_##name
#define STATE_EVENT_LOOP_STORE_T( name )    state_event_loop_store_##name##_t

#define STATE_EVENT_LOOP_STORE_DECL( name, num_sm, queue_size, max_registrations, max_event_id, stack_size, size_of_notif_block, size_of_posted_event )\
DISPATCH_QUEUE_STORE_DECL( name, queue_size, stack_size );\
NOTIFIER_STORE_DECL(name, max_event_id, max_registrations, size_of_notif_block);\
POOL_STORE_DECL(name, queue_size, size_of_posted_event, sizeof(void*));\
typedef struct {\
  state_event_loop_t event_loop;\
  state_machine_t sm[num_sm];\
  state_machine_create_params_t sm_create_params[num_sm];\
}STATE_EVENT_LOOP_STORE_T( name );

#define STATE_EVENT_LOOP_STORE_DEF( name )\
static DISPATCH_QUEUE_STORE_DEF( name );\
static NOTIFIER_STORE_DEF(name);\
static POOL_STORE_DEF(name);\
static STATE_EVENT_LOOP_STORE_T( name ) STATE_EVENT_LOOP_STORE( name )

#define STATE_EVENT_LOOP_CREATE_PARAMS_INIT( params, nm, task_name, task_priority, notifier_f, sm_names, start_states, client_data) do{\
  memset(&(params), 0, sizeof((params)));\
  (params).p_event_loop = &STATE_EVENT_LOOP_STORE(nm).event_loop;\
  (params).p_event_loop->p_sm = STATE_EVENT_LOOP_STORE(nm).sm;\
  CUTILS_ASSERTF(GetArraySize(sm_names) == GetArraySize(STATE_EVENT_LOOP_STORE(nm).sm),\
              "Number of state Names(%u) does not match num of state machines (%u)",\
               GetArraySize(sm_names), GetArraySize(STATE_EVENT_LOOP_STORE(nm).sm));\
  (params).num_machines = GetArraySize(STATE_EVENT_LOOP_STORE(nm).sm);\
  (params).p_sm = STATE_EVENT_LOOP_STORE(nm).sm;\
  (params).p_sm_create_params = STATE_EVENT_LOOP_STORE(nm).sm_create_params;\
  for(uint32_t iijj = 0; iijj < GetArraySize(sm_names); iijj++ ) {\
    (params).p_sm_create_params[iijj].p_state_mac = &STATE_EVENT_LOOP_STORE(nm).sm[iijj];\
    (params).p_sm_create_params[iijj].p_name = sm_names[iijj];\
    (params).p_sm_create_params[iijj].private_client_data = client_data;\
    (params).p_sm_create_params[iijj].start_state_id = start_states[iijj];\
    (params).p_sm_create_params[iijj].should_log = true;\
    (params).p_sm_create_params[iijj].log_level = 0;\
  }\
  (params).name = task_name;\
  NOTIFIER_STORE_CREATE_PARAMS_INIT((params).notifier_create_params, nm, notifier_f, task_name, true);\
  DISPATCH_QUEUE_CREATE_PARAMS_INIT((params).queue_params, nm, task_name, task_priority);\
  POOL_CREATE_INIT((params).event_pool_create_params, nm);\
  (params).exec_queue_priority = task_priority;\
  (params).event_data_size = (params).event_pool_create_params.element_size_requested;\
} while( 0 )

state_event_loop_t *state_event_loop_init(state_event_loop_create_params_t * create_params);
void state_event_loop_deinit(state_event_loop_t * event_loop);
void state_event_loop_add_state(state_event_loop_t * event_loop, state_t * state, uint32_t state_mac_id);
/**
 * @brief This allows the client of the state_event_loop to install a callback that is called prior to the
 * states or external listeners processing the event posted. The client can use this to update its global
 * data prior to other entities being notified. This callback must be installed prior to the state_event_loop
 * starting.
 * @param fn
 * @param client_data - Optional client data, typically a pointer to client context.
 * @return - True if callback successfully installed.
 */
bool state_event_loop_install_event_pre_proc(state_event_loop_t *event_loop, void (*fn)(event_t *, void *),
                                             void *client_data);
void state_event_loop_start(state_event_loop_t *event_loop);
void state_event_loop_stop(state_event_loop_t * event_loop);
state_event_registration_t *state_event_loop_allocate_registration(state_event_loop_t * event_loop);
bool state_event_loop_register_notification(state_event_loop_t * event_loop,
                                            uint32_t event_type, state_event_registration_t * p_reg);
void state_event_loop_deregister_notification(state_event_loop_t * event_loop,
                                              state_event_registration_t * p_reg);
uint32_t state_event_loop_get_current_state_id(state_event_loop_t* event_loop, uint32_t state_mac_index);
state_t* state_event_loop_get_current_state(state_event_loop_t* event_loop, uint32_t state_mac_index);

/**
 * @brief - will allocate the an event from the pool maintained internally in the event_loop. Clients can
 * use this as part of their event post routines.
 * @param event_loop - an initialized and running event loop
 * @return
 */
static inline event_t *state_event_loop_allocate_event(state_event_loop_t * event_loop)
{
  event_t *retval = 0;

  if (event_loop) {
    retval = pool_alloc(event_loop->p_pool_of_events);
    CUTILS_ASSERTF(retval, "Unable to acquire event from state_event_loop event Pool");
  }
  return retval;
}

/**
 * @brief Increases the reference count of the supplied event. This is a "protected" function
 * in that it is only meant to be called by the implementing client for internal use.
 * @param event_loop - an initialized and running event loop
 * @param event
 */
static inline void state_event_loop_retain_event(state_event_loop_t* event_loop, event_t* event)
{
  CUTILS_ASSERTF( event_loop && event, "Invalid Inputs" );
  pool_retain(event_loop->p_pool_of_events, event);
}

/**
 * @brief This is used to release an event. This is meant to be used internally by the implementing
 * client code
 * @param event_loop
 * @param event
 */
static inline void state_event_loop_release_event(state_event_loop_t* event_loop, event_t* event)
{
  CUTILS_ASSERTF( event_loop && event, "Invalid Inputs" );
  pool_free(event_loop->p_pool_of_events, event);
}

/**
 * @brief Will post an event that should be populated by the implementing client
 * @param event_loop
 * @param category
 * @param evt
 * @return
 */
static inline bool state_event_loop_post(state_event_loop_t * event_loop, uint32_t category, event_t * evt)
{
  bool retval = false;

  void state_event_loop_exec_queue_f(void *arg1, void *arg2);
  if (event_loop && evt) {
    event_t *event = state_event_loop_allocate_event(event_loop);

    CUTILS_ASSERT(event);
    memcpy(event, evt, event_loop->p_pool_of_events->element_size);
    event->event_loop = event_loop;
    event->event_id = category;
    event->to_string = evt->to_string;
    //TODO: Reenable This
    //system_log(&event_loop->log, event, "%s(%u): Posting" , __FUNCTION__, __LINE__);
    dispatch_async_f(event_loop->p_exec_queue, state_event_loop_exec_queue_f, event, NULL);
    retval = true;
  }
  return retval;
}

uint32_t state_event_loop_state_prefix(logger_t* p_logger, void* p_private, char* p_string, uint32_t string_size);

/**
 *  Macro to initalize a state data structure.
 *
 *  \param stId: uint32_t - a unique state Id
 *  \param stateName: const char* - state name string
 *  \param init: state_init_f - init function pointer
 *  \param Enter: state_enter_f - entry function pointer
 *  \param Exit: state_exit_f - exit function pointer
 *  \param validEvent: state_is_valid_event_f - valid event query
 *  \param handleEvent: state_handle_event_f - event handler
 *  \param logging: bool - Logging Enabled/Disabled
 */
#define STATE_INITIALIZER( stId, stateName, init, Enter, Exit, validEvent, handleEvent )\
{\
  .listElem = { 0 },\
  .stateId = stId,\
  .state_name = stateName,\
  .f_init = init,\
  .f_enter = Enter,\
  .f_exit = Exit,\
  .f_valid_event = validEvent,\
  .f_handle_event = handleEvent,\
  .entered_once = false,\
  .log.fnPrefix = state_event_loop_state_prefix,\
  .log.isEnabled = true,\
  .log.log_level = 0\
}

