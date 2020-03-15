[asyncio.h](../inc/cutils/asyncio.h) provides an abstraction to create asynchronous I/O over various I/O like interfaces. It encapsulates all the resources and the routing of tasks to provide an RX and TX channel. The Rx channel will be waiting on the read interface and the tx channel will be listening to a queue for buffers to send out. 

## Resources
The client is expected to provide all the necessary storage and the API provides macros to help with the creation of the resources statically. There are a significant number of resources that client will need to provide to setup the service, however the client need only focus on the business logic and leave the rest of the plumbing to the abstraction.

### Transmit resources 
On the transmit side, the API will maintain a pool of transmit buffers whose maximum size is parameterized and specified by the client. In addition the client will have to provide a dispatch queue on which the transmit functions can run

### Receive resources
The receive API will maintain a pool of buffers whose maximum size is parameterized and specified by the client. A dispatch queue will have to be provided by the client to run the rx functions.

### Read/Write callbacks
The Client will have to provide function callbacks to handle the reading/writing from the actual interfaces. 

#### [Write](../inc/cutils/asyncio.h#L108)
```
typedef size_t(*asyncio_interface_client_write_f)(asyncio_handle_t handle,
                                                  size_t tx_data_size,
                                                  uint8_t *p_tx_data_buf,
                                                  uint32_t timeout);
```

The client will write out the contents pointed to by `p_tx_data_buf` over the physical interface using `timeout` as a upper bound on the blocking if possible.

#### [Read](../inc/cutils/asyncio.h#L92)
```
typedef size_t (*asyncio_interface_client_read_f)(asyncio_handle_t handle,
                                                  uint8_t *pRxDataBuf,
                                                  uint32_t timeOut);
```

The client will read into the `pRxDataBuf`. The client should attempt to read upto the maximum size of the rx_buffers. If fewer bytes are received, it should exit specifying the number of bytes received in the return value.

#### [Rx Callback](../inc/cutils/asyncio.h#L63)
```
typedef void (*asyncio_rx_callback_f)(asyncio_handle_t h_asyncio, asyncio_message_t pMessage, size_t size);
```
This callback is called in the receiver thread, when the client supplied reader function returns a non-zero number of bytes. 

## Example Usage
Lets say we have some kind of UART interface which has a read/write functions of the form
```
size_t uart_read(void *buf, size_t size, uint32_t timeout_ms);
size_t uart_write(void *buf, size_t size, uint32_t timeout_ms);
```
The two functions will block on until the specified bytes have been received or a timeout occurs.

The asyncio api will create two threads, one for the transmit operation and another for the receive operation. Once the asyncio interface is initialized and started, the receiver will keep calling the client supplied read callback and the transmitter function will wait on a transmit request queue, sending data using the write callback when a request is received.

Creating an asyncio interface is similar to other APIs in the library. You created the static storage using the macros and then reference it when creating the asyncio interface in the client's initialization code.

Assume we will have transmit and receive buffers of 256 bytes max size and each needs to be aligned at on 16 bytes.
We expect both transmit and receive to hold 8 buffers. Worker threads wilil run with 8KB of stack space and will run at high priority.

### [Declaring](../inc/cutils/asyncio.h#L196) the storage type
In Global scope
```
ASYNCIO_STORE_DECL(someMacroIdentifier, 8, 8, 256, 256, 16);
```

### [Defining](../inc/cutils/asyncio.h#L208) an instance of the storage type
```
ASYNCIO_STORE_DEF(someMacroIdentifier);
```

### [Creating](../inc/cutils/asyncio.h#L213) the asyncio interface
In the clients initialization code:
```
//Assuming the clients provides the following definitions
static size_t client_write_f(asyncio_handle_t handle,
                             size_t tx_data_size,
                             uint8_t *p_tx_data_buf,
                             uint32_t timeout);
static size_t client_read_f(asyncio_handle_t handle,
                            uint8_t *pRxDataBuf,
                            uint32_t timeOut);
static void client_rx_f(asyncio_handle_t h_asyncio, asyncio_message_t pMessage, size_t size);
typedef struct {
  ...
  dispatch_queue_t *rx_queue; //These should be created as described in the dispatch_queue_t docs
  dispatch_queue_t *tx_queue; //These should be created as described in the dispatch_queue_t docs
  asyncio_handle_t h_asyncio;
  ...
}client_state_t;

void client_init(void)
{
  ...
  // Create the transmit and receive dispatch queues ahead of this.
  CUTILS_ASSERT(s_client_state.rx_queue && s_client_state.tx_queue);
  asyncio_create_params_t create_params;
  ASYNCIO_CREATE_PARAMS_INIT(create_params, 
                             someMacroIdentifier, 
                             "stream_name", 
                             client_read_f, 
                             client_write_f, 
                             client_rx_f,
                             s_client_state.rx_queue,
                             s_client_state.tx_queue,
                             s_client_state);
  s_client_state.h_asyncio = asyncio_create_instance(&create_params);
  CUTILS_ASSERT(s_client_state.h_asyncio);
}
```

