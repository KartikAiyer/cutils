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

#include <cutils/state_machine.h>
#include <string.h>

#define SM_LOG(str, ...)      console_log( &state_mac->logger, state_mac, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
//TODO: Reenable this
#define SM_SLOG(str, ...)
//system_log( &state_mac->logger, state_mac, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
#define SM_DEFAULT_LOGGING          (true)

static uint32_t StateMachinePrefix(logger_t * logger, void *priv, char *str, uint32_t str_size)
{
  uint32_t retval = 0;
  state_machine_t *state_mac = (state_machine_t *) priv;

  INSERT_TO_STRING(str, retval, "%s: %s", state_mac->p_name,
                   (state_mac->current_state) ? state_mac->current_state->state_name : "NoState");
  return retval;
}

static void state_machine_initialize(state_machine_t * pSm)
{
  memset(pSm, 0, sizeof(state_machine_t));
}

state_machine_t* state_machine_create(state_machine_create_params_t *params)
{
  state_machine_t* state_mac = 0;
  CUTILS_ASSERT(params && params->p_state_mac);
  state_mac = params->p_state_mac;
  state_machine_initialize(state_mac);
  state_mac->p_name = params->p_name;
  state_mac->logger.fnPrefix = StateMachinePrefix;
  state_mac->logger.isEnabled = params->should_log;
  state_mac->logger.log_level = params->log_level;
  state_mac->start_state_id = params->start_state_id;
  state_machine_set_private_data(state_mac, params->private_client_data);

  return state_mac;
}

state_t *state_machine_get_state(state_machine_t * state_mac, uint32_t stateId)
{
  state_t *pRetval = NULL;
  uint32_t i = 0;

  for (i = 0; i < state_mac->num_of_states; i++) {
    if (state_mac->states[i]->stateId == stateId) {
      pRetval = state_mac->states[i];
      break;
    }
  }

  return pRetval;
}

static inline void enter_state(state_machine_t *state_mac, state_t *state)
{
  if(!state->entered_once)  {
    state->entered_once = true;
    if( state->f_init ) state->f_init(state_mac, state);
  }
  state->f_enter(state_mac, state);
}

void state_machine_start(state_machine_t *state_mac)
{
  state_t *pInitialState = state_machine_get_state(state_mac, state_mac->start_state_id);

  // Set up the first state to load the state machine
  CUTILS_ASSERT(pInitialState);
  state_mac->current_state = pInitialState;

  // Call the init functions of all the states.

  state_mac->state_machine_started = true;
  // Enter the initial state.
  enter_state(state_mac, state_mac->current_state);
}

void state_machine_stop(state_machine_t * state_mac)
{
  if (state_mac) {
    SM_SLOG("Stopping State Machine");
    if (state_mac->current_state) {
      state_mac->current_state->f_exit(state_mac, state_mac->current_state);
      state_mac->current_state = 0;
      state_mac->transition_requested = false;
    }
    SM_SLOG("State Machine Stopped");
  }
}

void state_machine_register_state(state_machine_t * state_mac, state_t * state)
{
  if (state_mac && state && (state_mac->num_of_states < STATE_MAC_MAX_STATES)) {
    // Confirm presence of necessary functions
    CUTILS_ASSERT(state->f_enter);
    CUTILS_ASSERT(state->f_exit);
    CUTILS_ASSERT(state->f_valid_event);
    CUTILS_ASSERT(state->f_handle_event);
    state_mac->states[state_mac->num_of_states++] = state;
    state->p_sm = state_mac;
  }
}

void state_machine_handle_event(state_machine_t * state_mac, void *event)
{
  if (event && state_mac->state_machine_started) {
    if (state_mac->current_state->f_valid_event(state_mac, state_mac->current_state, event)) {
      state_mac->next_state_requested =
        state_mac->current_state->f_handle_event(state_mac, state_mac->current_state, event);

      // Will transition when event handling is completed and StateTransition() is invoked.
      if (state_mac->next_state_requested != state_mac->current_state->stateId) {
        state_mac->transition_requested = true;
      }
    }
  }
}

void state_machine_transition(state_machine_t * state_mac)
{
  if (state_mac->transition_requested && state_mac->state_machine_started) {
    state_mac->transition_requested = false;

    if (state_mac->current_state->stateId != state_mac->next_state_requested) {
      state_t *pNextState = state_machine_get_state(state_mac, state_mac->next_state_requested);

      if (!pNextState) {
        SM_SLOG("Did not find State: %d. FATAL!!!", state_mac->next_state_requested);
        CUTILS_ASSERT(pNextState);
      }
      SM_SLOG("Transition: %s -> %s", state_mac->current_state->state_name, pNextState->state_name);
      state_mac->current_state->f_exit(state_mac, state_mac->current_state);
      state_mac->current_state = pNextState;
      enter_state(state_mac, state_mac->current_state);
    }
  }
}

void* state_machine_get_private_data(state_machine_t* state_mac)
{
  CUTILS_ASSERTF(state_mac, "Valid State Machine required");
  return state_mac->p_private;
}

void state_machine_set_private_data(state_machine_t* state_mac, void* private)
{
  CUTILS_ASSERTF(state_mac, "Valid State Machine required");
  state_mac->p_private = private;
}
