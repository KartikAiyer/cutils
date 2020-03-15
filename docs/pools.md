A block pool implementation is provided in [pool.h](../inc/cutils/pool.h). This is a useful utility to provide fixed sized allocators. All storage is provided by the client. There are helper macros to create the backing storage for the pools. In addition the allocations from the pool support reference counting and allow the registration of an optional destructor per allocation.


## Example Usage
Assume you have a data structure like this
```
typedef struct {
  uint32_t id;
  char name[20];
}persont_t;
```
You want to create a pool of `person_t` so that you can use it as an allocator in an API that you are writing. Since you have to provide the backing storage for it you can expect to have some kind of static array that spans the storage requirements. You can use the pool static storage creation and manipulation macros to make it easier to create this static storage in your API compilation unit. 

The macros create a type that encapsulate the clients specific space requirements along with other accounting data. Thus the steps to creating the storage are __declaring__ a type for your storage, __defining__ an instance of that type and then using it to make a __create_params_t__ helper data structure that is used to create the actual pool. 

Assume you want to create a pool from which you can allocate upto 8 `persont_t` objects with each object being aligned on a 64 byte boundary.

### Declaring the storage type
In global scope you can do the following
```
POOL_STORE_DECL(someMacroIdentifier, 8, sizeof(person_t), 64);
```

### Define the type
Again in global scope
```
POOL_STORE_DEF(someMacroIdentifier);
```

The above macros can be composed in other macros that are trying to offer similar helper to statically allocate resources.

### Creating the pool

The pool can now be created in the clients initialization code
```
typedef struct {
  ...
  pool_t *person_pool;
}client_state_t;

void client_init(void)
{
...
   pool_create_params_t params;
   POOL_CREATE_INIT(params, someMacroIdentifier);
   s_client_state.person_pool = pool_create(&params);
...
}
``` 
