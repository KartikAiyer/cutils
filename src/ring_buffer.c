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

#include "cutils/ring_buffer.h"

#include <stdatomic.h>
#include <assert.h>
#include <string.h>

ring_buffer_ref create_ring_buffer(ring_buffer_create_params_t *params)
{
  struct ring_buffer *buffer = params->buffer;

  if (!buffer) {
    return NULL;
  }

  buffer->data = params->data;
  if (buffer->data == NULL) {
    return NULL;
  }

  buffer->size = params->size_in_bytes;
  return buffer;
}

void ring_buffer_release(ring_buffer_ref buffer)
{
  ring_buffer_reset_at_offset(buffer, 0);
}

bool ring_buffer_is_empty(ring_buffer_ref buffer)
{
  return ring_buffer_get_bytes_available_for_read(buffer) == 0;
}

bool ring_buffer_is_full(ring_buffer_ref buffer)
{
  return ring_buffer_get_bytes_available_for_write(buffer) == 0;
}

data_range_t ring_buffer_get_total_range(ring_buffer_ref buffer)
{
  return make_data_range(buffer->read_offset, buffer->size);
}

data_range_t ring_buffer_get_current_data_range(ring_buffer_ref buffer)
{
  return make_data_range(buffer->read_offset, ring_buffer_get_bytes_available_for_read(buffer));
}

uint32_t ring_buffer_get_bytes_available_for_read(ring_buffer_ref buffer)
{
  /*
   This unsynchronized access is perfectly safe.
   There's a race condition in that it's not certain whether the read thread will see the old or new value of _writeOffset.
   However, it doesn't matter. _writeOffset can only increase, and the number of available bytes can likewise only increase.
   If it sees the old value, it still computes a number of available bytes that's correct, just slightly out of date.
   If it sees the new value, then so much the better. Therefore, the reader is safe.
   */
  return (uint32_t) (buffer->write_offset - buffer->read_offset);
}

uint32_t ring_buffer_get_bytes_available_for_write(ring_buffer_ref buffer)
{
  /*
   it's safe for the reader thread to modify _readOffset while the writer is calculating bytesAvailableForRead.
   Advancing the read pointer decreases the number of available bytes, which increases the amount of write space.
   If the writer thread sees the old value for _readOffset here, it computes the older, smaller amount of write space, which is still safe, just slightly stale.
   */
  return buffer->size - ring_buffer_get_bytes_available_for_read(buffer);
}

uint32_t ring_buffer_get_size(ring_buffer_ref buffer)
{
  return buffer->size;
}

void ring_buffer_reset_at_offset(ring_buffer_ref buffer, uint32_t offset)
{
  buffer->read_offset = offset;
  buffer->write_offset = offset;
}

static inline uint32_t bufferReadOffsetWithOffset(ring_buffer_ref buffer, uint32_t offset)
{
  return (buffer->read_offset + offset) % buffer->size;
}

static inline uint32_t bufferWriteOffset(ring_buffer_ref buffer)
{
  return buffer->write_offset % buffer->size;
}

uint32_t ring_buffer_write_data(ring_buffer_ref buffer, const uint8_t *data, const uint32_t size)
{

  if (ring_buffer_is_full(buffer) || size == 0) {
    return 0;
  }

  const uint32_t file_write_offset = bufferWriteOffset(buffer);
  const uint32_t bytes_to_write = MIN(size, ring_buffer_get_bytes_available_for_write(buffer));
  const uint32_t remaining_write_bytes = buffer->size - file_write_offset;

  if (bytes_to_write > remaining_write_bytes) {
    // data wraps around, must write in 2 regions
    // 1.
    memcpy(buffer->data + file_write_offset, data, remaining_write_bytes);
    // 2.
    memcpy(buffer->data, data + remaining_write_bytes, bytes_to_write - remaining_write_bytes);

  } else {
    // contiguous
    const uint32_t bytes_to_copy = bytes_to_write < size ? bytes_to_write : size;
    memcpy(buffer->data + file_write_offset, data, bytes_to_copy);

  }

  // cache range read so it can be forwarded
  const data_range_t range_read = make_data_range(buffer->write_offset, bytes_to_write);

  // increment write index
  buffer->write_offset += bytes_to_write;

  // notify observers
  if (buffer->on_data_written_at_range) {
    buffer->on_data_written_at_range(buffer, buffer->ctx, range_read);
  }

  return bytes_to_write;
}

ring_buffer_data_t ring_buffer_get_data_at_range(ring_buffer_ref buffer, data_range_t requested_range, bool peek)
{
  const data_range_t current_range = ring_buffer_get_current_data_range(buffer);

  ring_buffer_data_t data = {0};

  if (ring_buffer_is_empty(buffer)
      || !location_in_data_range(requested_range.location, current_range)) {
    return data;
  }

  const bool update_read_offset = !peek;

  const uint32_t move_offset = (requested_range.location - current_range.location);
  const uint32_t file_read_offset = bufferReadOffsetWithOffset(buffer, move_offset);
  const uint32_t bytes_to_read = MIN(requested_range.length, current_range.length);
  const uint32_t remaining_read_bytes = buffer->size - file_read_offset;

  if (bytes_to_read > remaining_read_bytes) {
    // read wraps around, must read in 2 regions
    data.data[0].data = buffer->data + file_read_offset;
    data.data[0].length = remaining_read_bytes;

    data.data[1].data = buffer->data;
    data.data[1].length = bytes_to_read - remaining_read_bytes;

    data.entryCount = 2;

  } else {
    // contiguous block
    data.data[0].data = buffer->data + file_read_offset;
    data.data[0].length = bytes_to_read;

    data.entryCount = 1;
  }
  data.total_bytes = bytes_to_read;

  assert(bytes_to_read == data.data[0].length + data.data[1].length);
  assert(current_range.location + move_offset == requested_range.location);
  assert(requested_range.location + bytes_to_read <= max_data_range(current_range));

  // cache range read so it can be forwarded
  const data_range_t range_read = make_data_range(requested_range.location, bytes_to_read);

  // increment read pointer
  if (update_read_offset) {
    buffer->read_offset += bytes_to_read;
  }

  // notify observers
  if (buffer->on_data_read_at_range) {
    buffer->on_data_read_at_range(buffer, buffer->ctx, range_read);
  }

  return data;
}

ring_buffer_data_t ring_buffer_get_data(ring_buffer_ref buffer, uint32_t length, bool peek)
{
  const data_range_t read_range = make_data_range(buffer->read_offset, length);
  return ring_buffer_get_data_at_range(buffer, read_range, peek);
}

void ring_buffer_clear_data(ring_buffer_ref buffer, uint32_t length)
{
  const data_range_t current_range = ring_buffer_get_current_data_range(buffer);
  const uint32_t bytes_to_clear = MIN(length, current_range.length);
  buffer->read_offset += bytes_to_clear;
}

void ring_buffer_set_callback_context(ring_buffer_ref buffer, void *ctx)
{
  buffer->ctx = ctx;
}

void *ring_buffer_get_callback_context(ring_buffer_ref buffer)
{
  return buffer->ctx;
}

void ring_buffer_set_on_read_callback(ring_buffer_ref buffer, data_changed_at_range_f callback)
{
  buffer->on_data_read_at_range = callback;
}

void ring_buffer_set_on_write_callback(ring_buffer_ref buffer, data_changed_at_range_f callback)
{
  buffer->on_data_written_at_range = callback;
}

void ring_buffer_get_offsets(ring_buffer_ref buffer, uint32_t *size,
                             uint32_t *base_read_offset, uint32_t *write_offset)
{
  *size = buffer->size;
  *base_read_offset = buffer->read_offset;
  *write_offset = buffer->write_offset;
}
