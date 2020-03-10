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

#include <cutils/log_buffer.h>

static void log_buffer_char_push(log_buffer_t *pLb, char item);

static bool log_buffer_filter_character(log_buffer_t *pLb, char character)
{
  bool retval = true;

  // filter ansi escape sequences by looking for escape
  // start (0x1b) and escape end (letter character)
  if (0x1b == character) {
    pLb->in_escape_sequence = true;
  } else {
    if (pLb->in_escape_sequence) {
      if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
        pLb->in_escape_sequence = false;
      }
    } else {
      // filter out carriage return
      if (0xd != character) {
        retval = false;
      }
    }
  }

  return retval;
}

bool log_buffer_init(log_buffer_t *pLb, char *pBuffer, uint32_t bufferSize)
{
  bool retval = false;

  if (pLb && pBuffer && bufferSize) {
    pLb->p_buffer = pBuffer;
    pLb->bufferSize = bufferSize;
    pLb->head = pLb->tail = 0;
    pLb->isInit = true;
    retval = true;
  }
  return retval;
}

bool log_buffer_push(log_buffer_t *pLb, char *pString, uint32_t stringSize)
{
  bool retval = false;

  if (pLb && pLb->isInit && pString && stringSize < pLb->bufferSize) {
    for (uint32_t i = 0; i < stringSize; i++) {
      char character = pString[i];

      if (!log_buffer_filter_character(pLb, character)) {
        log_buffer_char_push(pLb, character);
      }
    }
    retval = true;
  }
  return retval;
}

bool log_buffer_char_pop(log_buffer_t *pLb, int8_t *pChar)
{
  bool retval = false;

  if (pLb && pChar && pLb->head != pLb->tail) {
    *pChar = pLb->p_buffer[pLb->tail];
    pLb->tail = ((pLb->tail + 1) < pLb->bufferSize) ? (pLb->tail + 1) : 0;
    retval = true;
  }
  return retval;
}

bool log_buffer_is_empty(log_buffer_t *pLb)
{
  bool retval = false;

  if (pLb && pLb->isInit) {
    retval = (pLb->head == pLb->tail);
  }
  return retval;
}

static void log_buffer_char_push(log_buffer_t *pLb, char item)
{
  if (pLb && item) {
    pLb->p_buffer[pLb->head] = item;
    pLb->head = ((pLb->head + 1) < pLb->bufferSize) ? (pLb->head + 1) : 0;
    if (pLb->head == pLb->tail) {
      // The Ring buffer was full... move the tail
      pLb->tail = ((pLb->tail + 1) < pLb->bufferSize) ? (pLb->tail + 1) : 0;
    }
  }
}

void log_buffer_clear(log_buffer_t *pLb)
{
  if (pLb && pLb->isInit) {
    pLb->head = pLb->tail = 0;
  }
}


uint32_t log_buffer_current_size(log_buffer_t *pLb,
                                 uint32_t *initial_chunk,
                                 uint32_t *residue_bytes)
{
  uint32_t ibytes = 0;
  uint32_t rbytes = 0;

  if (pLb->head > pLb->tail) {
    ibytes = pLb->head - pLb->tail;
    rbytes = 0;
  } else if (pLb->head < pLb->tail) {
    ibytes = pLb->bufferSize - pLb->tail;
    rbytes = pLb->head;
  }
  if (initial_chunk)
    *initial_chunk = ibytes;
  if (residue_bytes)
    *residue_bytes = rbytes;
  return (ibytes + rbytes);
}

uint32_t log_buffer_lines_from_size(uint32_t size)
{
  return  1 + ((size - 1) / LOG_MAX_LINE_LENGTH);
}
