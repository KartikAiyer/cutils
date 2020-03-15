# State Event Loop

The state event loop consolidates a set of state machines, a notifier and a dispatch queue to create an abstraction for an event loop. 
An event posted to the state_event_loop is processed in the following order within the state event loop's dispatch queue:
1. An optional pre-processor callback will first process the event. This callback can only be implemented by the implementer of the state_event_loop and 
   is not intended to be used by external clients.
2. Each state machine will sequentially process the event with each state machine processing and transitioning before the 
   next state machine processes the event.
3. The event will be dispatched to external listener dispatch queues (after incrementing the reference count on the event) and destroyed
   after being processed.
4. The event is destroyed.

