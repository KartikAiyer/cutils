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

#pragma once

#include <cutils/types.h>


/**
 *  @file LogBuffer 
 *  This Ring Buffer works for 1 byte characters to be used to
 *  capture logging.
 *  
 * 
 */

typedef struct _LogBuffer
{
  char *p_buffer;
  bool isInit;
  uint32_t bufferSize;
  uint32_t head, tail;
  bool in_escape_sequence;
} log_buffer_t;

/**
 * LogBufferInit - Initializes the ring buffer. 
 * 
 * \param pLb - Provide a valid Ring Buffer but don't fill it up
 * \param pBuffer - A backing store for the ring buffer
 * \param bufferSize - The total size of the backing buffer 
 */
bool log_buffer_init(log_buffer_t *pLb, char *pBuffer, uint32_t bufferSize);

/**
 * LogBufferPush - This will push the string character by 
 * character into the Log Buffer. 
 * 
 * \param pLb - Pointer to intialized Log Buffer 
 * \param pString - pString. 
 * \param stringSize - string size as determined by strlen()
 * 
 * \return bool - true if successful 
 */
bool log_buffer_push(log_buffer_t *pLb, char *pString, uint32_t stringSize);

/**
 * LogBufferIsEmpty - Returns true if the log buffer is empty. 
 *  
 * 
 * \param pLb 
 * 
 * \return bool 
 */
bool log_buffer_is_empty(log_buffer_t *pLb);

/**
 * LogBufferCharPop - Pops the character at the tail of the log buffer.
 * Returns true for a successful read and false if the Log buffer is empty.
 * 
 * \param pLb - pointer to valid Log Buffer
 * \param pChar - pointer to character which will contain popped value
 * \return bool - false if log buffer is empty or invalid inputs.
 */
bool log_buffer_char_pop(log_buffer_t *pLb, int8_t *pChar);

/**
 * LogBufferClear - Resets the Log buffer. Data is not invalidated but all pointers
 * are reset and the data will be overwritten. 
 * 
 * \param pLb - pointer to valid Log Buffer
 */
void log_buffer_clear(log_buffer_t *pLb);

uint32_t log_buffer_current_size(log_buffer_t *pLb,
                                 uint32_t *initial_chunk,
                                 uint32_t *residue_bytes);


uint32_t log_buffer_lines_from_size(uint32_t size);
