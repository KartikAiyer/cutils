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

#include <cutils/bst.h>
#include <cutils/logger.h>


void bst_insert(bst_t *bst, KListElem *elem)
{
  CUTILS_ASSERTF(bst, "Invalid BST");
  elem->prev = elem->next = 0;
  if (!bst->root) {
    bst->root = elem;
  } else {
    KListElem *current = bst->root;
    KListElem *parent = 0;
    while (current) {
      parent = current;
      if (bst->f_less(elem, current) < 0) {
        current = current->prev;
      } else {
        current = current->next;
      }
    }
    if (bst->f_less(elem, parent) < 0) {
      parent->prev = elem;
    } else {
      parent->next = elem;
    }
  }
}

KListElem *bst_search_inner(KListElem *root, KListElem *key, bst_less_f less)
{
  if (!root || !less(key, root)) {
    return root;
  } else if (less(key, root) < 0) {
    return bst_search_inner(root->prev, key, less);
  } else {
    return bst_search_inner(root->next, key, less);
  }
}

KListElem *bst_search(bst_t *bst, KListElem *key)
{
  CUTILS_ASSERTF(bst, "Invalid BST");
  return bst_search_inner(bst->root, key, bst->f_less);
}

static void bst_traverse_inner(KListElem *root, bst_traverse_f fn, va_list args)
{
  if (root) {
    bst_traverse_inner(root->prev, fn, args);
    fn(root, args);
    bst_traverse_inner(root->next, fn, args);
  }
}

void bst_traverse(bst_t *bst, bst_traverse_f fn, ...)
{
  va_list args;
  va_start(args, fn);
  bst_traverse_inner(bst->root, fn, args);
  va_end(args);
}
