/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2019> <Kartik Aiyer>
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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once
#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <cutils/types.h>
#include <stdio.h>
#include <string.h>


/**
 Holds the location / size of a chunk of data in the buffer.
 */
typedef struct
{
  uint32_t location;
  uint32_t length;
} data_range_t;

static inline data_range_t make_data_range(uint32_t loc, uint32_t len)
{
  data_range_t r;
  r.location = loc;
  r.length = len;
  return r;
}

static inline bool location_in_data_range(uint32_t loc, data_range_t range)
{
  return (!(loc < range.location) && (loc - range.location) < range.length) ? true : false;
}

static inline uint32_t max_data_range(data_range_t range)
{
  return (range.location + range.length);
}

/**
 Points to a chunk of data in the buffer.
 NOTE:
 The memory location pointed by data is owned by the buffer and NOT copied.
 You should copy this data if you plan to use it later, as the buffer may overwrite this location with new data.
 */
typedef struct
{
  uint8_t *data;
  uint32_t length;
} buffer_data_t;


/**
 For data reads, if the data wraps around, it will contain 2 sections of data, since the bytes are NOT copied.
 entryCount contains the amount of data regions set.
 totalBytes is the total aggregated amount of data of all data regions.
 */
typedef struct
{
  int entryCount;
  uint32_t total_bytes;
  buffer_data_t data[2];
} ring_buffer_data_t;

static inline void copy_ring_data_to_buffer(ring_buffer_data_t *data_to_copy, uint8_t *dst)
{
  if (data_to_copy->entryCount == 0) {
    return;
  }

  memcpy(dst, data_to_copy->data[0].data, data_to_copy->data[0].length);
  if (data_to_copy->entryCount == 2) {
    memcpy(dst + data_to_copy->data[0].length, data_to_copy->data[1].data, data_to_copy->data[1].length);
  }
}

/**
 Opaque type for a MemRingBuffer
 */
typedef struct ring_buffer *ring_buffer_ref;

/**
 Callback signature for read / write notification callbacks.
 */
typedef void (*data_changed_at_range_f)(ring_buffer_ref buffer, void *ctx, data_range_t range);

typedef struct ring_buffer
{
  uint32_t size;

  /*
   Sequentially consistent is the default type for classic C-style operations on _Atomic() variables.
   _Atomic(int) a = 42;
   ...
   a++; // <- This is sequentially consistent
   http://www.informit.com/articles/article.aspx?p=1832575&seqNum=4
   */
  atomic_ullong read_offset;   // start offset for available data. these monotonically increase and do not wrap around
  atomic_ullong write_offset;

  // ptr to the mapped region
  uint8_t *data;

  // callbacks;
  void *ctx;
  data_changed_at_range_f on_data_read_at_range;
  data_changed_at_range_f on_data_written_at_range;
}ring_buffer_t;

#define MEM_RING_BUFFER_STORE(name)   __mem_ring_buffer_store_##name
#define MEM_RING_BUFFER_STORE_T(name) __mem_ring_buffer_store_##name##_t
#define MEM_RING_BUFFER_STORE_DECL(name, sizeInBytes) \
typedef struct {\
  ring_buffer rb;\
  uint8_t buffer[sizeInBytes]; \
}MEM_RING_BUFFER_STORE_T(name)

#define MEM_RING_BUFFER_STORE_DEF(name)\
MEM_RING_BUFFER_STORE_T(name) MEM_RING_BUFFER_STORE(name)

typedef struct {
  struct ring_buffer *buffer;
  uint8_t *data;
  uint32_t size_in_bytes;
}ring_buffer_create_params_t;

#define MEM_RING_BUFFER_CREATE_PARAMS_INIT(params, name)\
do {\
  (params).buffer = &MEM_RING_BUFFER_STORE(name).rb;\
  (params).data = MEM_RING_BUFFER_STORE(name).buffer;\
  (params).size_in_bytes = GetArraySize(MEM_RING_BUFFER_STORE(name).buffer);\
}while(0)

/**
 Creates a new memory mapped buffer of the given size.

 @param sizeInBytes the size in bytes of the buffer
 @return an empty buffer ready to be used, or NULL in case of an error
 */
ring_buffer_ref create_ring_buffer(ring_buffer_create_params_t *params);


/**
 Returns the mapped memory and frees the memory used by the buffer. NULL safe.

 @param buffer the buffer to free.
 */
void ring_buffer_release(ring_buffer_ref buffer);


/**
 Tests to see if a buffer is empty

 @param buffer the buffer to query
 @return true if the buffer is empty (no data)
 */
bool ring_buffer_is_empty(ring_buffer_ref buffer);


/**
 Tests to see if a buffer is full

 @param buffer the buffer to query
 @return true if the buffer is full (can't fit anymore data)
 */
bool ring_buffer_is_full(ring_buffer_ref buffer);


/**
 Queries a buffer for its size

 @param buffer the buffer to query
 @return the size in bytes of the buffer.
 */
uint32_t ring_buffer_get_size(ring_buffer_ref buffer);


/**
 Gets the entire possible range of the buffer: [readOffset, size]

 @param buffer the buffer to query
 @return the current, total range for the buffer
 */
data_range_t ring_buffer_get_total_range(ring_buffer_ref buffer);


/**
 Gets the current valid data range.

 @param buffer the buffer to query
 @return the data range containing valid data.
 */
data_range_t ring_buffer_get_current_data_range(ring_buffer_ref buffer);


/**
 Gets the total amount of bytes available for reading, that is from the current read position to the end of the valid data.

 @param buffer the buffer to query
 @return the total amount of bytes available for reading.
 */
uint32_t ring_buffer_get_bytes_available_for_read(ring_buffer_ref buffer);


/**
 Gets the amount of "free" bytes still available in the buffer

 @param buffer the buffer to query
 @return the total amount of bytes that can still be written to the buffer.
 */
uint32_t ring_buffer_get_bytes_available_for_write(ring_buffer_ref buffer);


/**
 Writes data into the buffer, if there's space available.

 @param buffer the buffer to write data into
 @param data pointer to the start of the data to write
 @param size amount of bytes to copy from @c data
 @return the amount of bytes that were actually written into the buffer.
 */
uint32_t ring_buffer_write_data(ring_buffer_ref buffer, const uint8_t *data, const uint32_t size);


/**
 Reads the maximum amount of data available at the given range.
 The returned data can either be empty (data pointers null) or be less data than the requested amount,
 in case the buffer only had a portion of the requested data

 @param buffer the buffer to read from
 @param requested_range range of the data to read
 @param peek if true, the read offset is NOT modified by this read operation.
 @return the requested data. The data is referenced, not copied. See @c MappedData for how to retrieve it.
 */
ring_buffer_data_t ring_buffer_get_data_at_range(ring_buffer_ref buffer, data_range_t requested_range, bool peek);


/**
 Reads the maximum amount of data available at the current offset (up to length).
 The returned data can either be empty (data pointers null) or be less data than the requested amount,
 in case the buffer only had a portion of the requested data

 @param buffer the buffer to read from
 @param length amount of data to read
 @param peek if true, the read offset is NOT modified by this read operation.
 @return the requested data. The data is referenced, not copied. See @c MappedData for how to retrieve it.
 */
ring_buffer_data_t ring_buffer_get_data(ring_buffer_ref buffer, uint32_t length, bool peek);


/**
 clears up to length bytes from the buffer. If the data size is less than length the minimum of the two is cleared

 @param buffer the buffer to clear from
 @param length amount of data to clear
 */
void ring_buffer_clear_data(ring_buffer_ref buffer, uint32_t length);

/**
 Logically resets the buffer at a new offset. The buffer will be "empty" after this call.

 @param buffer the buffer to reset
 @param offset starting position for the buffer. The new range will be [offset, size]
 */
void ring_buffer_reset_at_offset(ring_buffer_ref buffer, uint32_t offset);


/**
 Sets the context that will be passed to callbacks.

 @param buffer the buffer for which to set the context
 @param ctx arbitrary data that will be passed as a parameter in the data callbacks
 */
void ring_buffer_set_callback_context(ring_buffer_ref buffer, void *ctx);

/**
 Gets the context that will be passed to callbacks.
 */
void *ring_buffer_get_callback_context(ring_buffer_ref buffer);


/**
 If set, will be invoked after each read operation to notify the range of the read data.

 @param buffer the buffer to observe
 @param callback the callback function to invoke on reads
 */
void ring_buffer_set_on_read_callback(ring_buffer_ref buffer, data_changed_at_range_f callback);


/**
 If set, will be invoked after each write operation to notify the range of the written data.

 @param buffer the buffer to observe
 @param callback the callback function to invoke on writes
 */
void ring_buffer_set_on_write_callback(ring_buffer_ref buffer, data_changed_at_range_f callback);


/**
 Gets offsets from the mappedBuffer (clever)
 */
void ring_buffer_get_offsets(ring_buffer_ref buffer, uint32_t *size,
                             uint32_t *base_read_offset, uint32_t *write_offset);

#ifdef __cplusplus
}
#endif
/* MemRingBuffer_h */
