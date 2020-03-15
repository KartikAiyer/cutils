The [dispatch queue](../inc/cutils/dispatch_queue.h) API aims to provide something similar to [Apple's dispatch queues](https://developer.apple.com/documentation/dispatch/dispatchqueue). As in the apple API a dispatch queue is a FIFO on which a client can submit tasks to be run. The dispatch queue is associated with a thread underneath on which the tasks are run. 

All the storage for a dispatch queue is provided by the client and macros are provided to help with creation of said storage. Resources include stack space for the thread, pool space for the total number of tasks that can be queued and the fifo itself.  

Tasks that can be queued must have a specific [signature](../inc/cutils/os_types.h#L50)
```
typedef void (*dispatch_function_t)(void *arg1, void *arg2);
```
The above prototype should be generic enough for most use cases. Note that the dispatch queue will only allocate storage to hold the pointers to the arguments but will not be creating the actual storage for the memory pointed to. The client is expected to own that. 
## Usage Example

### Creating the dispatch queue
As with pools, the client create the static storage for the dispatch queue by __declaring__ the type that represents the storage requirements, __defining__ an instance of the type and then using that type to make a __create_params_t__ helper object which is used to create the dispatch queue.

Assume you want to create a dispatch queue using stack space of 8KB and with the ability to queue upto 20 function pointers. 

### Declare the storage type
In global scope
```
DISPATCH_QUEUE_STORE_DECL(someMacroIdentifier, 20, 8 * 1024);
```

### Define and instance of the storage type
Again in global scope
```
DISPATCH_QUEUE_STORE_DEF(someMacroIdentifier);
```

### Creating the dispatch queue
Now that the type and instance has been created, the client can create the dispatch queue in its initialization code

```
tyepdef struct {
  ...
  dispatch_queue_t* task_queue;
  ...
}client_state_t;

void client_init(void) 
{
  ...
  dispatch_queue_create_params_t queue_create_params;
  DISPATCH_QUEUE_CREATE_PARAMS_INIT(queue_create_params, someMacroIdentifier, "client_task_name", CUTILS_TASK_PRIORITY_MEDIUM);
  s_client_state.task_queue = dispatch_queue_create(&queue_create_params);
  CUTILS_ASSERT(s_client_state.task_queue);
  ...
}
```

### Posting an action on the queue.
Any entity that has a valid handle to a dispatch queue in say `p_queue` can post an action on the queue. 

```
static void client_action(void* arg1, void* arg2)
{
  ...
}

void client_foo(void)
{
  ...
  //Assume p_queue is a valid dispatch_queue_t* 
  dispatch_async_f(p_queue, client_action, arg1, arg2);
  ...
}
```