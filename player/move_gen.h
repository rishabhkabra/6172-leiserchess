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

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "util.h"

#define MAX_NUM_MOVES 100      // real number = 7 x (8 + 3) + 1 x (8 + 4) = 89
#define MAX_PLY_IN_SEARCH 100  // up to 100 ply
#define MAX_PLY_IN_GAME 4096  // long game!  ;^)

// Used for debugging and display
#define MAX_CHARS_IN_MOVE 16  // Could be less
#define MAX_CHARS_IN_TOKEN 64

// the board (which is 10x10) is centered in a 16x16 array
#define ARR_WIDTH 16
#define ARR_SIZE (ARR_WIDTH * ARR_WIDTH)

// board is 10 x 10
#define BOARD_WIDTH 10

typedef int square_t;
typedef int rnk_t;
typedef int fil_t;

#define FIL_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)
#define RNK_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)

#define FIL_SHIFT 4
#define FIL_MASK 15
#define RNK_SHIFT 0
#define RNK_MASK 15


//----------------------------------------------------------------------
// pieces

// IMP: Aligning this to 8 gives improvement.
#define PIECE_SIZE 5  // Number of bits in (ptype, color, orientation)

typedef int piece_t;

//----------------------------------------------------------------------
// piece types

#define PTYPE_SHIFT 2
#define PTYPE_MASK 3

typedef enum {
    EMPTY,
    PAWN,
    KING,
    INVALID
} ptype_t;

//----------------------------------------------------------------------
// colors

#define COLOR_SHIFT 4
#define COLOR_MASK 1

typedef enum {
    WHITE = 0,
    BLACK
} color_t;

//----------------------------------------------------------------------
// Orientations

#define NUM_ORI 4
#define ORI_SHIFT 0
#define ORI_MASK (NUM_ORI - 1)

typedef enum {
  NN,
  EE,
  SS,
  WW
} king_ori_t;

typedef enum {
  NW,
  NE,
  SE,
  SW
} pawn_ori_t;

//----------------------------------------------------------------------
// moves

#define KO (-1)  // returned by make move in illegal ko situation

// MOVE_MASK is 20 bits
#define MOVE_MASK 0xfffff

#define PTYPE_MV_SHIFT 18
#define PTYPE_MV_MASK 3
#define FROM_SHIFT 8
#define FROM_MASK 0xFF
#define TO_SHIFT 0
#define TO_MASK 0xFF
#define ROT_SHIFT 16
#define ROT_MASK 3

typedef uint32_t move_t;
typedef uint64_t sortable_move_t;

// Rotations
typedef enum {
  NONE,
  RIGHT,
  UTURN,
  LEFT
} rot_t;


//----------------------------------------------------------------------
// position

typedef struct position {
  piece_t      board[ARR_SIZE];
  struct position  *history;     // history of position
  uint64_t     key;              // hash key
  int          ply;              // Even ply are White, odd are Black
  move_t       last_move;        // move that led to this position
  piece_t      victim;           // piece destroyed by shooter
  square_t     king_locs[2];          // location of kings
} position_t;


// Function prototypes
char * color_to_str(color_t c);
color_t color_to_move_of(position_t *p);
color_t color_of(piece_t x);
color_t opp_color(color_t c);
void set_color(piece_t *x, color_t c);
//ptype_t ptype_of(piece_t x);

inline ptype_t ptype_of(piece_t x) {
  return (ptype_t) ((x >> PTYPE_SHIFT) & PTYPE_MASK);
}

void set_ptype(piece_t *x, ptype_t pt);
// int ori_of(piece_t x);
void set_ori(piece_t *x, int ori);
void init_zob();
//square_t square_of(fil_t f, rnk_t r);

inline square_t square_of(fil_t f, rnk_t r) {
  square_t s = ARR_WIDTH * (FIL_ORIGIN + f) + RNK_ORIGIN + r;
  DEBUG_LOG(1, "Square of (file %d, rank %d) is %d\n", f, r, s);
  assert((s >= 0) && (s < ARR_SIZE));
  return s;
}

inline fil_t fil_of(square_t sq) {
  fil_t f = ((sq >> FIL_SHIFT) & FIL_MASK) - FIL_ORIGIN;
  DEBUG_LOG(1, "File of square %d is %d\n", sq, f);
  return f;
}

inline rnk_t rnk_of(square_t sq) {
  rnk_t r = ((sq >> RNK_SHIFT) & RNK_MASK) - RNK_ORIGIN;
  DEBUG_LOG(1, "Rank of square %d is %d\n", sq, r);
  return r;
}

inline square_t from_square(move_t mv) {
  return (mv >> FROM_SHIFT) & FROM_MASK;
}

inline square_t to_square(move_t mv) {
  return (mv >> TO_SHIFT) & TO_MASK;
}

inline rot_t rot_of(move_t mv) {
  return (rot_t) ((mv >> ROT_SHIFT) & ROT_MASK);
}

inline move_t move_of(ptype_t typ, rot_t rot, square_t from_sq, square_t to_sq) {
  return ((typ & PTYPE_MV_MASK) << PTYPE_MV_SHIFT) |
         ((rot & ROT_MASK) << ROT_SHIFT) |
         ((from_sq & FROM_MASK) << FROM_SHIFT) |
         ((to_sq & TO_MASK) << TO_SHIFT);
}

inline int ori_of(piece_t x) {
  return (x >> ORI_SHIFT) & ORI_MASK;
}

fil_t fil_of(square_t sq);
rnk_t rnk_of(square_t sq);
int square_to_str(square_t sq, char *buf);
int dir_of(int i);
int beam_of(int direction);
int reflect_of(int beam_dir, int pawn_ori);
// square_t from_square(move_t mv);
// square_t to_square(move_t mv);
// rot_t rot_of(move_t mv);
// move_t move_of(ptype_t typ, rot_t rot, square_t from_sq, square_t to_sq);
void move_to_str(move_t mv, char *buf);
int generate_all(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict);
void do_perft(position_t *gme, int depth, int ply);
void low_level_make_move(position_t *previous, position_t *next, move_t mv);
piece_t make_move(position_t *previous, position_t *next, move_t mv);
void display(position_t *p);
uint64_t compute_zob_key(position_t *p);

#endif  // MOVE_GEN_H
