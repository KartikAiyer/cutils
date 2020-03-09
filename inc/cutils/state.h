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

#include <cutils/types.h>
#include <cutils/klist.h>
#include <cutils/logger.h>

// /Forward Declarations
struct _state_t;
struct _state_machine_t;

// /Callback Prototypes

/**
 * The state_init_f callback function is called after the specific state has been created.
 *  
 * The function assumes that the State passed is allocated of 
 * the appropriate type. ie. If you call funtion pointer on say 
 * StateReady type object, you need to pass the same StateReady 
 * object in as a parameter. 
 *  
 *  
 * \param pState State* An allocated and initialized state 
 *                      sruct.
 */
typedef void (*state_init_f) (struct _state_machine_t * p_sm, struct _state_t * pState);

/**
 * Function pointer is called when entering the state. 
 *  
 * \param pState State* A valid state pointer 
 */
typedef void (*state_enter_f) (struct _state_machine_t * p_sm, struct _state_t * pState);

/**
 * Funtion pointer is called when exiting the state
 * 
 * \param pState State* A valid State pointer
 */
typedef void (*state_exit_f) (struct _state_machine_t * p_sm, struct _state_t * pState);

/**
 * Query function which responds true if the event can be 
 * handled by the state. 
 *  
 * \param pState State* A Valid state pointer 
 * \param pEvetn AppEvent* pointer to event received. 
 *  
 * \return bool True if event can be handled false otherwise.
 */
typedef bool(*state_is_valid_event_f) (struct _state_machine_t * p_sm, struct _state_t * pState,
                                       void *pEvent);

/**
 * Function pointer to State Event Handler. 
 *  
 * The function handles the incoming event and returns state ID 
 * to transition to. The returned state Id can be the current 
 * state indicating that no state transition is required. 
 *  
 * \param pState State* Pointer to current State 
 * \param pEvent AppEvent* Pointer to received event. 
 *  
 * \return uint32_t A desitination stateId. This can be the 
 *         current stateId as well thus implying no state
 *         transition required.
 */
typedef uint32_t(*state_handle_event_f) (struct _state_machine_t * p_sm, struct _state_t * pState,
                                         void *pEvent);

/**
 * \struct State 
 * \brief Data structure that encapsulates an application state. 
 *  
 * The data structure is the base representation of a state 
 * which holds all the generic state handling function pointers. 
 * Specific states should have an instance of this data 
 * structure as the first memeber of their data structure. 
 * eg.
  
 * typedef struct _StateReady 
 * {
 *   State base;
 *   bool isReady;
 * }StateReady; 
 *  
 * The data structure can be added to a Klist. Refer to klist.h 
 */
typedef struct _state_t
{
  /** List elem so that it can be queued in a list. Must be
   *  first element */
  KListElem listElem;
  /** a unique stateId across states to be used in a state
   *  machine */
  uint32_t stateId;
  /** Static string as state name */
  const char *state_name;
  /** Pointer to init function (Optional). This function is called
   *  once when the State machine is started. This function is
   *  optiona Pointer to init function. This function is called
   *  once when the State machine is started. This function is
   *  optional
   */
  state_init_f f_init;
  /** Pointer to function to be called when state is entered. */
  state_enter_f f_enter;
  /** Poiunter to function when state is exited. */
  state_exit_f f_exit;
  /** Pointer to query function that checks if a state can
   *  handle a specific event. */
  state_is_valid_event_f f_valid_event;
  /** Pointer to State specific event handler. */
  state_handle_event_f f_handle_event;
  /** A pointer to the state machine that the state is installed in.
   *  Set when the state is added to the state machine.
   */
  struct _state_machine_t *p_sm;
  logger_t log;
  bool entered_once;
} state_t;

#define STATE_CLOG(state, p_event, str, ...)\
console_log(&((state)->log), p_event, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

//TODO: Renable this
#define STATE_SLOG(state, p_event, str, ...)
//system_log(&((state)->log), p_event, "%s(%u)" str, __FUNCTION__, __LINE__, ##__VA_ARGS__)
