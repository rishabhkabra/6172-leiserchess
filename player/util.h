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

#ifndef UTIL_H
#define UTIL_H


#ifndef DEBUG_VERBOSE
#define DEBUG_VERBOSE 0
#endif 

#if DEBUG_VERBOSE
// DEBUG logging thresh level; 
// When calling debug_log, only messages with log_level >= DEBUG_LOG_THRESH get printed
// so "important" debugging messages should get higher level
#define DEBUG_LOG_THRESH 3
#define DEBUG_LOG(arg...) debug_log(arg)
#define WHEN_DEBUG_VERBOSE(ex) ex
#else
#define DEBUG_LOG_THRESH 0
#define DEBUG_LOG(arg...)
#define WHEN_DEBUG_VERBOSE(ex)
#endif // EVAL_DEBUG_VERBOSE


void debug_log(int log_level, const char *str, ...);
double  milliseconds();
uint64_t myrand();

#endif  // UTIL_H
