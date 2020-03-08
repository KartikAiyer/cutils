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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <cutils/klist.h>

typedef int (*bst_less_f)(KListElem *elem1, KListElem *elem2);

typedef void (*bst_traverse_f)(KListElem *elem, va_list args);

typedef struct _bst_t
{
  KListHead *root;
  bst_less_f f_less;
} bst_t;


static inline void bst_init(bst_t *bst, bst_less_f f_less)
{
  if (bst && f_less) {
    bst->f_less = f_less;
    bst->root = 0;
  }
}

void bst_insert(bst_t *bst, KListElem *elem);

KListElem *bst_search(bst_t *bst, KListElem *key);

void bst_traverse(bst_t *bst, bst_traverse_f fn, ...);
