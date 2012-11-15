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

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#include "util.h"


void debug_log(int log_level, const char *errstr, ...) {
  if (log_level >= DEBUG_LOG_THRESH) {
    va_list arg_list;
    va_start(arg_list, errstr);
    vfprintf(stderr, errstr, arg_list);
    va_end(arg_list);
    fprintf(stderr, "\n");
  }
}

double milliseconds() {
  struct timeval t;

  gettimeofday(&t, NULL);

  double result = 1000.0 * t.tv_sec;
  result += t.tv_usec/1000.0;

  return result;
}

// Public domain code for JLKISS64 RNG - long period KISS RNG producing
// 64-bit results
uint64_t myrand() {
  static int first_time = 0;
  // Seed variables
  static uint64_t x = 123456789123ULL, y = 987654321987ULL;
  static unsigned int z1 = 43219876, c1 = 6543217, z2 = 21987643,
                      c2 = 1732654;  // Seed variables
  static uint64_t t;

  if (first_time) {
    int  i;
    FILE *f = fopen("/dev/urandom", "r");
    for (i = 0; i < 64; i += 8) {
      x = x ^ getc(f) << i;
      y = y ^ getc(f) << i;
    }

    fclose(f);
    first_time = 0;
  }

  x = 1490024343005336237ULL * x + 123456789;

  y ^= y << 21;
  y ^= y >> 17;
  y ^= y << 30;  // Do not set y=0!

  t = 4294584393ULL * z1 + c1;
  c1 = t >> 32;
  z1 = t;

  t = 4246477509ULL * z2 + c2;
  c2 = t >> 32;
  z2 = t;

  return x + y + z1 + ((uint64_t)z2 << 32);  // Return 64-bit result
}

