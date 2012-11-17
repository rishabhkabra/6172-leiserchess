/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include "./abort.hpp"
#include "./fake_mutex.h"
cilkscreen::fake_mutex abort_mutex;
Abort::Abort() : aborted(false), parent() {
  pollGranularity = 1;
  count = 0;
}

Abort::Abort(Abort *p) : aborted(false), parent(p) {
    pollGranularity = 1;
    count = 0;
}
void Abort::setGranularity(int newGranularity) {
    pollGranularity = newGranularity;
}

int Abort::isAborted() {
  abort_mutex.lock();
  // allows for polling every i'th call
  if (count >= pollGranularity - 1) {
    count = 0;
  } else {
    count++;
    abort_mutex.unlock();
    return 0;
  }
  bool ret = aborted || (parent && this->parent->isAborted());
  abort_mutex.unlock();
  return ret;
}

void Abort::abort() {
  abort_mutex.lock();
  #ifdef STATS
  if (!aborted) {
    abort_count += 1;
  }
  #endif
  aborted = true;
  abort_mutex.unlock();
}
