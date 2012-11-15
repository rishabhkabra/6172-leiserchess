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


// Search
#ifndef SEARCH_H
#define SEARCH_H

#include "move_gen.h"

// score_t values
#define INF 32700
#define WIN 32000
#define PAWN_VALUE 100

// the maximum possible value for score_t type
#define MAX_SCORE_VAL INT16_MAX
typedef int16_t score_t;  // Search uses "low res" values

void init_killer();
void init_tics();
void init_abort_timer(double goal_time);
double elapsed_time();
bool should_abort();
void reset_abort();

void init_best_move_history();
move_t get_move(sortable_move_t sortable_mv);
score_t searchRoot(position_t *p, score_t alpha, score_t beta, int depth,
                   int ply, move_t *pv, uint64_t *node_count, FILE *OUT);

#endif  // SEARCH_H
