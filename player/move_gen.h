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
#include <iostream>
#include <bitset>
#include "util.h"

#define MAX_NUM_MOVES 100      // real number = 7 x (8 + 3) + 1 x (8 + 4) = 89
#define MAX_PLY_IN_SEARCH 100  // up to 100 ply
#define MAX_PLY_IN_GAME 4096  // long game!  ;^)

// Used for debugging and display
#define MAX_CHARS_IN_MOVE 16  // Could be less
#define MAX_CHARS_IN_TOKEN 64

// the board (which is 10x10) is centered in a 16x16 array
#define ARR_WIDTH 12
#define ARR_SIZE (ARR_WIDTH * ARR_WIDTH)

// Pawns array size (number of pawns for each color)
#define PAWNS_COUNT 7

// board is 10 x 10
#define BOARD_WIDTH 10

typedef int square_t;
typedef char rnk_t;
typedef char fil_t;

#define FIL_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)
#define RNK_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)

#define FIL_SHIFT 4
#define FIL_MASK 15
#define RNK_SHIFT 0
#define RNK_MASK 15

#define CACHE_LINE_ALIGNMENT __declspec(align(64))
//----------------------------------------------------------------------
// pieces

// IMP: Aligning this to 8 gives improvement.
#define PIECE_SIZE 5  // Number of bits in (ptype, color, orientation)

typedef char piece_t;

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

#define NUM_ORIENTATION 4
#define ORIENTATION_SHIFT 0
#define ORIENTATION_MASK (NUM_ORIENTATION - 1)

typedef enum {
  NN,
  EE,
  SS,
  WW
} king_orientation_t;

typedef enum {
  NW,
  NE,
  SE,
  SW
} pawn_orientation_t;

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
//TODO: Figure out the optimal ordering of this.

typedef struct position {
  uint64_t     key;              // hash key
  move_t       last_move;        // move that led to this position
  piece_t      victim;           // piece destroyed by shooter
  square_t     king_locs[2];     // location of kings
  struct position  *history;     // history of position
  short int    ply;              // Even ply are White, odd are Black
  piece_t      board[ARR_SIZE];
  square_t     pawns_locs[2][PAWNS_COUNT + 1]; // Locations of the pawns
  //bitset<BITS_PER_BOARD_SIDE>     bit_ranks[BOARD_WIDTH];
  //bitset<BITS_PER_BOARD_SIDE>     bit_files[BOARD_WIDTH];
  uint16_t bit_ranks[BOARD_WIDTH];
  uint16_t bit_files[BOARD_WIDTH];
} position_t;

#define BITS_PER_VECTOR 16

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
// int orientation_of(piece_t x);
void set_ori(piece_t *x, int ori);
void init_zob();
//square_t square_of(fil_t f, rnk_t r);

const unsigned char square_of_table[10][10] = {
{'\xd', '\xe', '\xf', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16'},
{'\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22'},
{'\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e'},
{'\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a'},
{'\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46'},
{'\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52'},
{'\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e'},
{'\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a'},
{'\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76'},
{'\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f', '\x80', '\x81', '\x82'},
};

inline square_t square_of(fil_t f, rnk_t r) {
  square_t s = ARR_WIDTH * (FIL_ORIGIN + f) + RNK_ORIGIN + r;
  // square_t s = (square_t)(square_of_table[f][r]);
  assert((square_t) (ARR_WIDTH * (FIL_ORIGIN + f) + RNK_ORIGIN + r) ==  s);
  DEBUG_LOG(1, "Square of (file %d, rank %d) is %d\n", f, r, s);
  assert((s >= 0) && (s < ARR_SIZE));
  return s;
}

const unsigned char fil_of_table[144] = {
  '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa',
  '\xa', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\xa',
  '\xa', '\x1', '\x1', '\x1', '\x1', '\x1', '\x1', '\x1', '\x1', '\x1', '\x1', '\xa',
  '\xa', '\x2', '\x2', '\x2', '\x2', '\x2', '\x2', '\x2', '\x2', '\x2', '\x2', '\xa',
  '\xa', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3', '\xa',
  '\xa', '\x4', '\x4', '\x4', '\x4', '\x4', '\x4', '\x4', '\x4', '\x4', '\x4', '\xa',
  '\xa', '\x5', '\x5', '\x5', '\x5', '\x5', '\x5', '\x5', '\x5', '\x5', '\x5', '\xa',
  '\xa', '\x6', '\x6', '\x6', '\x6', '\x6', '\x6', '\x6', '\x6', '\x6', '\x6', '\xa',
  '\xa', '\x7', '\x7', '\x7', '\x7', '\x7', '\x7', '\x7', '\x7', '\x7', '\x7', '\xa',
  '\xa', '\x8', '\x8', '\x8', '\x8', '\x8', '\x8', '\x8', '\x8', '\x8', '\x8', '\xa',
  '\xa', '\x9', '\x9', '\x9', '\x9', '\x9', '\x9', '\x9', '\x9', '\x9', '\x9', '\xa',
  '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa',
  };


inline fil_t fil_of(square_t sq) {
  fil_t f = (fil_t) (fil_of_table[sq]);
  // fil_t f = ((sq >> FIL_SHIFT) & FIL_MASK) - FIL_ORIGIN;
  assert((fil_t)((sq / ARR_WIDTH) - FIL_ORIGIN) == f);
  DEBUG_LOG(1, "File of square %d is %d\n", sq, f);
  // printf("fil: %d\n", f);
  return f;
}

const unsigned char rnk_of_table[144] = {
  '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7', '\x8', '\x9', '\xa',
  '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa', '\xa',
  };

inline rnk_t rnk_of(square_t sq) {
  rnk_t r = (rnk_t) (rnk_of_table[sq]);
  assert((rnk_t)((sq % ARR_WIDTH) - RNK_ORIGIN) == r);
  DEBUG_LOG(1, "Rank of square %d is %d\n", sq, r);
  // printf("rank: %d\n", r);
  return r;
}

inline bool is_greater_than_zero (square_t sq) { 
  return (sq > 0); 
}

inline void reset_bit(position *p, fil_t f, rnk_t r) {
  
  p->bit_ranks[r] &= ~(1 << (BITS_PER_VECTOR - f - 1)); // reset r'th column's f'th bit to 0. bitmask needed is all ones except f'th bit.                 
  p->bit_files[f] &= ~(1 << (BITS_PER_VECTOR - r - 1)); // reset f'th row's r'th bit to 0
  /*
  p->bit_ranks[r].reset(f);
  p->bit_files[f].reset(r);
  */
}

inline void set_bit(position *p, fil_t f, rnk_t r) {
  
  p->bit_ranks[r] |= 1 << (BITS_PER_VECTOR - f - 1); // set to_r'th column's to_f'th bit to 1                                                                         
  p->bit_files[f] |= 1 << (BITS_PER_VECTOR - r - 1); // set to_f'th row's to_r'th bit to 1                                                                            
  /*
  p->bit_ranks[r].set(f);
  p->bit_ranks[f].set(r);
  */
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

inline int orientation_of(piece_t x) {
  return (x >> ORIENTATION_SHIFT) & ORIENTATION_MASK;
}

inline ptype_t ptype_mv_of(move_t mv) {
  return (ptype_t) ((mv >> PTYPE_MV_SHIFT) & PTYPE_MV_MASK);
}

fil_t fil_of(square_t sq);
rnk_t rnk_of(square_t sq);
int square_to_str(square_t sq, char *buf);
int dir_of(int i);

// directions for laser: NN, EE, SS, WW                                                                                                                                             
static int beam[NUM_ORIENTATION] = {1, ARR_WIDTH, -1, -ARR_WIDTH};

inline int beam_of(int direction) {
  assert(direction >= 0 && direction < NUM_ORIENTATION);
  return beam[direction];
}

// reflect[beam_dir][pawn_orientation]                                                                                                                                              
// -1 indicates back of Pawn                                                                                                                                                        
static int reflect[NUM_ORIENTATION][NUM_ORIENTATION] = {
  //  NW  NE  SE  SW                                                                                                                                                                
  { -1, -1, EE, WW},   // NN = beam_dir 0                                                                                                                                           
  { NN, -1, -1, SS},   // EE = beam_dir 1                                                                                                                                           
  { WW, EE, -1, -1 },  // SS = beam_dir 2                                                                                                                                           
  { -1, NN, SS, -1 }   // WW = beam_dir 3                                                                                                                                           
};

inline int reflect_of(int beam_dir, int pawn_orientation) {
  assert(beam_dir >= 0 && beam_dir < NUM_ORIENTATION);
  assert(pawn_orientation >= 0 && pawn_orientation < NUM_ORIENTATION);
  return reflect[beam_dir][pawn_orientation];
}
bool is_move_valid(position_t *p, move_t mv);
ptype_t ptype_mv_of(move_t mv);
void move_to_str(move_t mv, char *buf);
int generate_all(position_t *p, sortable_move_t *sortable_move_list);
void do_perft(position_t *gme, int depth, int ply);
piece_t make_move(position_t *previous, position_t *next, move_t mv);
void display(position_t *p);
uint64_t compute_zob_key(position_t *p);

#endif  // MOVE_GEN_H
