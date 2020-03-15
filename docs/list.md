[klist.h](../inc/cutils/klist.h) is a linked list implementation where the storage for the linked list element is provided by the entry that is being stored in the list. 

This is accomplished using the inheritance pattern available in C. Clients that want to store data in the linked list should represent that data as a `struct` whose first member is `KListElem` type.

Prepend and poping the first element off the list are O(1). The API is provided as a set of macros. This list pattern in used in some of the abstractions provided in this library. 
 
## Usage Example
Assume we have some data coming from a Gyro in the form
```
typedef struct {
  uint32_t gyro_x;
  uint32_t gyro_y;
  uint32_t gyro_z;
}gyro_data_t;
```

We can create a linked list insertable structure as follows:
```
tyepdef struct {
  KListElem elem;
  gyro_data_t data;
}gyro_data_elem_t;
```

The module that owns the list will have to maintain a pointer to the head of the list which is initialized to NULL, representing an empty list.
```
struct client_owning_list_t {
   ...
   KListHead *p_head;
};
```

To insert a gyro data element, the client will allocate an instance of `gyro_data_elem_t` and fill in the gyro data. It will then insert into the list as such:
```
KLIST_HEAD_PREPEND(client_owning_list.p_head, &gyro_data->elem);
```

Note the `p_head` pointer will be appropriately modified to reflect the current head pointer. 

Similarly to pop an element off use
```
KLIST_HEAD_POP(p_head, p_elem)
```
where `p_elem` is of type `KListElem*`.

There are other macros provided to iterate through the list and find elements in the list.
