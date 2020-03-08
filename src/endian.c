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

#include <cutils/endian.h>


uint16_t byteswap16(uint16_t x)
{
  return (uint16_t) (x << 8 | x >> 8);
}

uint32_t byteswap32(uint32_t x)
{
  return (((x ^ (x >> 16 | (x << 16))) & 0xFF00FFFF) >> 8) ^ (x >> 8 | x << 24);
}

uint64_t byteswap64(uint64_t x)
{
  union
  {
    uint64_t ull;
    uint32_t ul[2];
  } u;

  /* This actually generates the best code */
  u.ul[0] = (uint32_t) (x >> 32);
  u.ul[1] = (uint32_t) (x & 0xffffffff);
  u.ul[0] = byteswap32(u.ul[0]);
  u.ul[1] = byteswap32(u.ul[1]);
  return u.ull;
}

