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

// Transposition table

#ifndef TT_H
#define TT_H

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "move_gen.h"
#include "search.h"


typedef enum {
  UPPER,
  LOWER,
  EXACT
} ttBound_t;

// Just forward declarations
// The real definition is in tt.c
typedef struct ttRec ttRec_t;

// accessor methods for accessing move and score recorded in ttRec_t
move_t tt_move_of(ttRec_t *tt);
score_t tt_score_of(ttRec_t *tt);

size_t tt_get_bytes_per_record();
uint32_t tt_get_num_of_records();

// operations on the global hashtable
void tt_make_hashtable(int sizeMeg);
void tt_resize_hashtable(int sizeInMeg);
void tt_free_hashtable();
void tt_age_hashtable();

// putting / getting transposition data into / from hashtable
void tt_hashtable_put(uint64_t key, int depth, score_t score,
                      int type, move_t move);
ttRec_t *tt_hashtable_get(uint64_t key);

score_t tt_adjust_score_from_hashtable(ttRec_t *rec, int ply);
score_t tt_adjust_score_for_hashtable(score_t score, int ply);
bool tt_is_usable(ttRec_t *tt, int depth, score_t beta);

#endif  // TT_H
