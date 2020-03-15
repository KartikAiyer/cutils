[Notifier](../inc/cutils/notifier.h) provides an abstraction that allows clients to register callbacks keyed to a `uint32_t` id. The API allows the calling of all callbacks associated with a specific ID. If the IDs represent enumerated messages, then the notifier table represents a set of callbacks keyed by message ID. Each ID can store a linked list of callbacks. So when a specific ID is invoked, the entire callback chain for the ID is called one by one. 

One way to think of this is to imagine an array indexed by the message id and each value of the array is a linked list of callbacks (in fact this is how it is implemented).

Note that this uses a mutex to synchronize entries into the callback table. As in the other cases, the storage for notifier is provided by the client and helper macros are provided to assist the application writer in creating these storage objects statically. 

## Usage Example

Assume you have some messages coming over some interface. You have __4__ different message types and you don't want to write all the handling for each message received in one file. Instead you would like each of the modules that are interested in a specific message to be able to register a callback which is called when the message arrives. This way context data does not have to be shared all over the place and unnecessary public functions don't have to be created. You also want to allow the registration of upto __60__ callbacks (given that we use static allocation, you will have to think about maximums that can be tuned as a compile time option). 

Here is a sample message definition
```
typedef enum {
  eMessage1,
  eMessage2,
  eMessage3,
  eMessage4,
  eMessageMax
}message_type_e

typedef struct {
  message_type_e id;
  union {
     struct {
       uint32_t val1;
     }message_data_1;
     struct {
       uint32_t val2;
     }message_data_2;
  }data;
}message_t
```
The implementer wants to provide its clients with a callback as such
```
typedef void message_callback_f(message_t* message, void* private_data);
```
ie. the client will provide a function pointer and an optional pointer to some pre-allocated context. The notifier implementer will have to remember those items until the notification is deleted by the client. 

### Implementer requirements
#### Registration Blocks
The notifier will create a pool which will hold the information needed to represent a single callback registration. `notifier_block_t` is the base representation of this registration data. The implementation will have to define a data structure that inherits (c-style) from `notifier_block_t` and when creating notifier use the derived data structure as the notification block. In this derived `notifier_block_t` the implementer can store details of its specific callback.

```
typedef struct {
  notifier_block_t base;
  message_callback_f fn;
  void *private_data;
}implementer_notifier_block_t;
```
#### Notification delegate callback
Notifier merely handles the boilerplate of registering and calling callbacks. It does not try to make any assumptions about the data that is being posted through the callbacks or the signature of the callback itself. Instead it expects the implementer of a notifier to implement the following callback and invoke the client callbacks from within that callback. The callback is called whenever `notifier_post_notification()` is called.
```
typedef void (*notifier_f) (notifier_block_t * block, uint32_t category, void *notification_data);
```

In the context of our example this could look like
```
//Category refers to the uint32_t message id. ie it would be message_type_e. This is the message_id that was posted
//to the notifier. 
static void implementer_notification(notifier_block_t* block, uint32_t category, void* notification_data)
{
  implementer_notifier_block_t* imp_block = (implementer_notifier_block_t*)block;
  message_t* message = (message_t*)notification_data;
  imp_block->fn(message, imp_block->private_data); 
}
```

### Creating the notifier
#### [Declare storage type](../inc/cutils/notifier.h#L109)
In global space
```
NOTIFIER_STORE_DECL(someMacroIdentifer, eMessageMax, 60, sizeof(client_notifier_block_t));
```
#### [Define an instance of the storage type](../inc/cutils/notifier.h#L118)
In global scope
```
NOTIFIER_STORE_DEF(someMacroIdentifier);
```
#### [Creating the notifier](../inc/cutils/notifier.h#L125)
In the implementation initialization code you will create the notifier

```
typedef struct {
   ...
   notifier_t *notifier;
}message_notifier_t;

void messages_notifier_init(void)
{
  ...
  notifier_create_params create_params;
  NOTIFIER_STORE_CREATE_PARAMS_INIT(create_params, 
                                    someMacroIdentifier, 
                                    implementer_notification, 
                                    "message notifier", 
                                    true);
  s_message_notifier.notifier = notifier_init(&create_params);
  CUTILS_ASSERT(s_message_notifier.notifier);
  ...
}
``` 

#### Registering a notification
The implementer will provide a function to register a callback as such
```
typedef void* registration_h
registration_h implementer_register_message_notification(message_e message_id, message_callback_f fn, void* private_data)
{
   ...
   implementer_notifier_block_t *p_block = 
             (implementer_notifier_block_t *)notifier_allocate_notification_block(s_message_notifier.notifier);
   p_block->fn = fn;
   p_block->private_data = private_data;
   CUTILS_ASSERT(notifier_register_notification_block(s_message_notifier.notifier, (uint32_t)message_id, p_block));
   return (registration_h)p_block;
}
```
#### Posting a notification
The implementer will provide a function to post a pre-allocated `message_t`
```
void implementer_post_message(message_t* message)
{
  notifier_post_notification(s_message_notifier.notifier, message->id, message);
}
```
