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

#include "state.h"
/** Maximum number of states that a state machine can have. */
#define STATE_MAC_MAX_STATES        20

/**
 * \struct StateMachine 
 * \brief Encapsulates a state machine 
 *  
 * This data structure encapsulates data specific to a state 
 * machine. A state machine contains an array of state pointers, 
 * which are registered with the state machine during the state 
 * machine's configuration. The configuration steps include 
 * 1) Create the state machine 
 * 2) Configure the State machine with all the states 
 * 3) Start the state machine. 
 *  
 */
typedef struct _state_machine_t
{
  char *p_name;
  /** Number of states in the state machine. Used when
   *  iterating through state array */
  uint32_t num_of_states;
  /** Pointer to the current state. This is passed to the
   *  appropriate state functions and event handlers. */
  state_t *current_state;
  /** Array of all states in the state machine. */
  state_t *states[STATE_MAC_MAX_STATES];
  /** Marked true when a state transition is detected. State
   *  transitions are detected in the event handler. */
  bool transition_requested;
  /** The next state requested. This is always updated in the
   *  state handler and either points to the current state or
   *  the next State when requested. */
  uint32_t next_state_requested;
  void *p_private;
  bool state_machine_started;
  uint32_t start_state_id;
  logger_t logger;
} state_machine_t;

typedef struct _state_machine_create_params_t
{
  state_machine_t* p_state_mac;
  char* p_name;
  void* private_client_data;
  uint32_t start_state_id;
  bool should_log;
  uint32_t log_level;
}state_machine_create_params_t;


state_machine_t* state_machine_create(state_machine_create_params_t *params);

/**
 * State specific event handler 
 *  
 * The function will call the state specific event handler and 
 * will latch any state transition requested. 
 * 
 * 
 * @param pStateMac: StateMachine* - pointer to valid state
 *                 machine that has been started.
 * @param pEvent: AppEvent* - pointer to valid Event 
 */
void state_machine_handle_event(state_machine_t * state_mac, void *event);

/**
 * Register state with state machine.
 *  
 * The function registers a fully allocated and initialized 
 * state with the state machine. Should be called before 
 * starting the state machine. 
 * 
 * 
 * @param pStateMac: StateMachine* - Pointer to valid state 
 *                 machine
 * @param pState: State* - Pointer to valid State 
 */
void state_machine_register_state(state_machine_t * state_mac, state_t * state);

/** 
 * Get state for requested stateId from state machine 
 *   
 *  Returns the State* structure for the requested stateId
 *  (based on what was registered), from the provided state
 *  machine.
 *  
 * @param pStateMac: StateMachine* - pointer to valid state 
 *                 machine
 * @param stateId: uint32_t - valid state Id. 
 * 
 * @return State* - pointer to state structure requested or NULL
 *         if not found.
 */
state_t *state_machine_get_state(state_machine_t * state_mac, uint32_t state_id);

/**
 * Start the state machine. 
 *  
 * This will load the Inital state into the state machine and 
 * thus hook it up to the event handler. 
 * 
 * 
 * @param pStateMac: StateMachine* - pointer to valid state 
 *                 machine
 * @param initialState: uint32_t - valid initial state id.
 */
void state_machine_start(state_machine_t *state_mac);

/**
 * StateMachineStop - This will stop the state machine. All 
 * subsequent events posted will be discarded. Calling this is 
 * not so simple. You should call this in the same context that 
 * the State Machine Events are handled, or you have to 
 * synchronoize between the state machine event hander thread 
 * and the poster of this. This will effectively stop the state 
 * machine from engaging.  
 * 
 * 
 * @param pStateMac -  
 */
void state_machine_stop(state_machine_t * state_mac);

/**
 * Executes state transition.
 * 
 *  Will execute a state transition if one is requested. The
 *  function is called by the event handler when it is ready to
 *  execute a state transition.
 *  
 * @param pStateMac: StateMachine* - pointer to valid state 
 *                 machine.
 */
void state_machine_transition(state_machine_t * state_mac);

/**
 * @brief States machines can be supplied with client data which can retrieved
 * from the state machine using this API. This will be useful in the client specific
 * states
 * @param state_mac - A Valid State Machine
 * @return void* - NULL if no private data or the private data supplied in the create params
 */
void* state_machine_get_private_data(state_machine_t* state_mac);

/**
 * @brief Clients supply private data to the state machine using this API. This can be retrieved
 * using `state_machine_get_private_data()` in client specific state code.
 * @param state_mac - Valid State machine pointer
 * @param private - Private data
 */
void state_machine_set_private_data(state_machine_t* state_mac, void* private);
