A [state machine](../inc/cutils/state_machine.h) skeletal abstraction is provided which can be used to implement specific state machine and its corresponding states. 

The API simply exposes a container of states and a small set of functions to manipulate the container (post an event and transition states). The API is intended to be contained and used in a larger more context aware framework such as an application event runner or a specific state based operation. 

[States](../inc/cutils/state.h) are objects that are typically written as separate files and must implement five callbacks

```
typedef void (*state_init_f) (struct _state_machine_t * p_sm, struct _state_t * p_state);
```
A lazy initializer that is invoked when the state is entered for the fist time.
  
```
typedef void (*state_enter_f) (struct _state_machine_t * p_sm, struct _state_t * p_state);
```
Invoked when the state machine enters the specific state.
```
typedef void (*state_exit_f) (struct _state_machine_t * p_sm, struct _state_t * p_state);
```
Invoked when the state machine exits the specific state
```
typedef bool(*state_is_valid_event_f) (struct _state_machine_t * p_sm, 
                                       struct _state_t * p_state,
                                       void *p_event);
```
Invoked when an event is posted to query if the state actually processes the event. 
```
typedef uint32_t(*state_handle_event_f) (struct _state_machine_t * p_sm, 
                                         struct _state_t * pState,
                                         void *pEvent);
```
Invoked when an event is posted and `state_is_valid_event_f` returns true for the event. The function should return the next state the state machine should transition to. To stay in the same state, it returns its state id.

