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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "fen.h"
#include "move_gen.h"
#include "search.h"
#include "util.h"

#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

int USE_KO;  // Respect the Ko rule

static char *color_strs[2] = {"White", "Black"};

char * color_to_str(color_t c) {
  return color_strs[c];
}

// colors accessors
color_t color_to_move_of(position_t *p) {
  if ((p->ply & 1) == 0) {
    return WHITE;
  } else {
    return BLACK;
  }
}

color_t color_of(piece_t x) {
  return (color_t) ((x >> COLOR_SHIFT) & COLOR_MASK);
}

color_t opp_color(color_t c) {
  if (c == WHITE) {
    return BLACK;
  } else {
    return WHITE;
  }
}


void set_color(piece_t *x, color_t c) {
  assert((c >= 0) & (c <= COLOR_MASK));
  *x = ((c & COLOR_MASK) << COLOR_SHIFT) |
        (*x & ~(COLOR_MASK << COLOR_SHIFT));
}

void set_ptype(piece_t *x, ptype_t pt) {
  *x = ((pt & PTYPE_MASK) << PTYPE_SHIFT) |
        (*x & ~(PTYPE_MASK << PTYPE_SHIFT));
}

void set_ori(piece_t *x, int ori) {
  *x = ((ori & ORIENTATION_MASK) << ORIENTATION_SHIFT) |
        (*x & ~(ORIENTATION_MASK << ORIENTATION_SHIFT));
}

// King orientations
char *king_orientation_to_rep[2][NUM_ORIENTATION] = { { "NN", "EE", "SS", "WW" },
                                      { "nn", "ee", "ss", "ww" } };

// Pawn orientations
char *pawn_orientation_to_rep[2][NUM_ORIENTATION] = { { "NW", "NE", "SE", "SW" },
                                      { "nw", "ne", "se", "sw" } };

char *nesw_to_str[NUM_ORIENTATION] = {"north", "east", "south", "west"};


//----------------------------------------------------------------------
// Board, squares

static uint64_t   zob[ARR_SIZE][1<<PIECE_SIZE];
static uint64_t   zob_color;

static inline void check_bit_row_and_column(position_t * p) {
  for (int i = 0; i < BOARD_WIDTH; i++) {
    uint16_t column = 0;
    for (int j = 0; j < BOARD_WIDTH; j++) {
      bool col_value = (p->bit_ranks[j] & (1 << (BITS_PER_VECTOR - i - 1))) != 0;
      if (col_value) {
        column++;
      }
      column = column << 1;
    }
    column = column << 5;
    assert(p->bit_files[i] == column);
  }

  for (int f = 0; f < BOARD_WIDTH; f++) {
    for (int r = 0; r < BOARD_WIDTH; r++) {
      assert(ptype_of(p->board[square_of(f,r)]) != INVALID);
      if (ptype_of(p->board[square_of(f,r)]) == EMPTY) {
        assert((p->bit_files[f] & (1 << (BITS_PER_VECTOR - r - 1))) == 0);
        assert((p->bit_ranks[r] & (1 << (BITS_PER_VECTOR - f - 1))) == 0);
      } else {
        assert((p->bit_files[f] & (1 << (BITS_PER_VECTOR - r - 1))) != 0);
        assert((p->bit_ranks[r] & (1 << (BITS_PER_VECTOR - f - 1))) != 0);
      }
    }
  }
}

// Zobrist
uint64_t compute_zob_key(position_t *p) {
  uint64_t key = 0;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t sq = square_of(f, r);
      key ^= zob[sq][p->board[sq]];
    }
  }
  if (color_to_move_of(p) == BLACK)
    key ^= zob_color;

  return key;
}

uint64_t myrand();

void init_zob() {
  for (int i = 0; i < ARR_SIZE; i++) {
    for (int j = 0; j < (1 << PIECE_SIZE); j++) {
      zob[i][j] = myrand();
    }
  }
  zob_color = myrand();
}

// For no square, use 0, which is guaranteed to be off board
/*square_t square_of(fil_t f, rnk_t r) {
  square_t s = ARR_WIDTH * (FIL_ORIGIN + f) + RNK_ORIGIN + r;
  DEBUG_LOG(1, "Square of (file %d, rank %d) is %d\n", f, r, s);
  assert((s >= 0) && (s < ARR_SIZE));
  return s;
}*/

// converts a square to string notation, returns number of characters printed
int square_to_str(square_t sq, char *buf) {
  fil_t f = fil_of(sq);
  rnk_t r = rnk_of(sq);
  if (f >= 0) {
    return sprintf(buf, "%c%d", 'a'+ f, r);
  } else  {
    return sprintf(buf, "%c%d", 'z' + f + 1, r);
  }
}

// direction map
static int dir[8] = { -ARR_WIDTH - 1, -ARR_WIDTH, -ARR_WIDTH + 1, -1, 1,
                              ARR_WIDTH - 1, ARR_WIDTH, ARR_WIDTH + 1 };

int dir_of(int i) {
  assert(i >= 0 && i < 8);
  return dir[i];
}


// directions for laser: NN, EE, SS, WW
static int beam[NUM_ORIENTATION] = {1, ARR_WIDTH, -1, -ARR_WIDTH};

int beam_of(int direction) {
  assert(direction >= 0 && direction < NUM_ORIENTATION);
  return beam[direction];
}

// reflect[beam_dir][pawn_orientation]
// -1 indicates back of Pawn
int reflect[NUM_ORIENTATION][NUM_ORIENTATION] = {
//  NW  NE  SE  SW
  { -1, -1, EE, WW},   // NN = beam_dir 0
  { NN, -1, -1, SS},   // EE = beam_dir 1
  { WW, EE, -1, -1 },  // SS = beam_dir 2
  { -1, NN, SS, -1 }   // WW = beam_dir 3
};

int reflect_of(int beam_dir, int pawn_orientation) {
  assert(beam_dir >= 0 && beam_dir < NUM_ORIENTATION);
  assert(pawn_orientation >= 0 && pawn_orientation < NUM_ORIENTATION);
  return reflect[beam_dir][pawn_orientation];
}

// converts a move to string notation for FEN
void move_to_str(move_t mv, char *buf) {
  square_t f = from_square(mv);  // from-square
  square_t t = to_square(mv);    // to-square
  rot_t r = rot_of(mv);          // rotation

  buf += square_to_str(f, buf);
  if (f != t) {
    buf += square_to_str(t, buf);
  } else {
    switch (r) {
     case NONE:
      buf += square_to_str(t, buf);
      break;
     case RIGHT:
      buf += sprintf(buf, "R");
      break;
     case UTURN:
      buf += sprintf(buf, "U");
      break;
     case LEFT:
      buf += sprintf(buf, "L");
      break;
     default:
      assert(false);  // Bad, bad, bad
      break;
    }
  }
}

// Generate all moves from position p.  Returns number of moves.
// strict currently ignored.
// Noe being called anymore, for testing purposes.
int generate_all_old(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict) {
  color_t ctm = color_to_move_of(p);
  int move_count = 0;

  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t  sq = square_of(f, r);
      piece_t x = p->board[sq];
      ptype_t typ = ptype_of(x);
      if (typ == EMPTY) {
        continue;
      }
      assert(typ != INVALID);
      color_t color = color_of(x);
      if (color != ctm) {  // Wrong color
        continue;
      }
      // directions
      for (int d = 0; d < 8; d++) {
        int dest = sq + dir_of(d);
        if (ptype_of(p->board[dest]) == INVALID) {
          continue;    // illegal square
        }

        WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
        WHEN_DEBUG_VERBOSE({
          move_to_str(move_of(typ, 0, sq, dest), buf);
          DEBUG_LOG(1, "Before: %s ", buf);
        })
        assert(move_count < MAX_NUM_MOVES);
        sortable_move_list[move_count++] = move_of(typ, (rot_t) 0, sq, dest);

        WHEN_DEBUG_VERBOSE({
          move_to_str(get_move(sortable_move_list[move_count-1]), buf);
          DEBUG_LOG(1, "After: %s\n", buf);
        })
      }

      // rotations - three directions possible
      for (int rot = 1; rot < 4; ++rot) {
        assert(move_count < MAX_NUM_MOVES);
        sortable_move_list[move_count++] = move_of(typ, (rot_t) rot, sq, sq);
      }
      if (typ == KING) {  // Also generate null move
        assert(move_count < MAX_NUM_MOVES);
        sortable_move_list[move_count++] = move_of(typ, (rot_t) 0, sq, sq);
      }
    }
  }

  WHEN_DEBUG_VERBOSE({
    DEBUG_LOG(1, "\nGenerated moves: ");
    for (int i = 0; i < move_count; ++i) {
      char buf[MAX_CHARS_IN_MOVE];
      move_to_str(get_move(sortable_move_list[i]), buf);
      DEBUG_LOG(1, "%s ", buf);
    }
    DEBUG_LOG(1, "\n");
  })

  return move_count;
}

inline int generate_single_piece_move(ptype_t typ, square_t sq, int move_count, position_t *p, sortable_move_t *sortable_move_list) {
  assert(typ != INVALID);
  // directions
  for (int d = 0; d < 8; d++) {
    int dest = sq + dir_of(d);
    if (ptype_of(p->board[dest]) == INVALID) {
      continue;    // illegal square
    }

    WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
    WHEN_DEBUG_VERBOSE({
      move_to_str(move_of(typ, 0, sq, dest), buf);
      DEBUG_LOG(1, "Before: %s ", buf);
    })
    assert(move_count < MAX_NUM_MOVES);
    sortable_move_list[move_count++] = move_of(typ, (rot_t) 0, sq, dest);

    WHEN_DEBUG_VERBOSE({
      move_to_str(get_move(sortable_move_list[move_count-1]), buf);
      DEBUG_LOG(1, "After: %s\n", buf);
    })
  }
  // rotations - three directions possible
  for (int rot = 1; rot < 4; ++rot) {
    assert(move_count < MAX_NUM_MOVES);
    sortable_move_list[move_count++] = move_of(typ, (rot_t) rot, sq, sq);
  }
  return move_count;
}

int generate_all(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict) {
  color_t ctm = color_to_move_of(p);
  int move_count = 0;
  // Generate kings move
  square_t sq = p->king_locs[ctm];
  move_count = generate_single_piece_move(KING, sq, move_count, p, sortable_move_list);
  assert(move_count < MAX_NUM_MOVES);
  sortable_move_list[move_count++] = move_of(KING, (rot_t) 0, sq, sq); // Also generate null move
  for (int i = 0; i < PAWNS_COUNT; i++) {
    sq = p->pawns_locs[ctm][i];
    if (!sq) {
      continue;
    }
    move_count = generate_single_piece_move(PAWN, sq, move_count, p, sortable_move_list);
  }
  return move_count;
}

int generate_all_test(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict) {
  sortable_move_t new_gen_list[MAX_NUM_MOVES];
  for (int i = 0; i < MAX_NUM_MOVES; i++) {
    sortable_move_list[i] = 0;
    new_gen_list[i] = 0;
  }
  int move_count = generate_all_old(p, sortable_move_list, strict);
  int move_count_new = generate_all(p, new_gen_list, strict);
  assert(move_count == move_count_new);
  std::sort(sortable_move_list, sortable_move_list + MAX_NUM_MOVES);
  std::sort(new_gen_list, new_gen_list + MAX_NUM_MOVES);
  std::reverse(sortable_move_list, sortable_move_list + MAX_NUM_MOVES);
  std::reverse(new_gen_list, new_gen_list + MAX_NUM_MOVES);
  for (int i = 0; i < MAX_NUM_MOVES; i++) {
    assert(sortable_move_list[i] == new_gen_list[i]);
  }
  return move_count;
}


void low_level_make_move(position_t *previous, position_t *next, move_t mv) {
  assert(mv != 0);

  WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
  WHEN_DEBUG_VERBOSE({
    move_to_str(mv, buf);
    DEBUG_LOG(1, "low_level_make_move: %s\n", buf);
  })

  assert(previous->key == compute_zob_key(previous));

  WHEN_DEBUG_VERBOSE({
    fprintf(stderr, "Before:\n");
    display(previous);
  })

  square_t from_sq = from_square(mv);
  square_t to_sq = to_square(mv);
  rot_t rot = rot_of(mv);

  WHEN_DEBUG_VERBOSE({
    DEBUG_LOG(1, "low_level_make_move 2:\n");
    square_to_str(from_sq, buf);
    DEBUG_LOG(1, "from_sq: %s\n", buf);
    square_to_str(to_sq, buf);
    DEBUG_LOG(1, "to_sq: %s\n", buf);
    switch(rot) {
      case NONE:
        DEBUG_LOG(1, "rot: none\n");
        break;
      case RIGHT:
        DEBUG_LOG(1, "rot: R\n");
        break;
      case UTURN:
        DEBUG_LOG(1, "rot: U\n");
        break;
      case LEFT:
        DEBUG_LOG(1, "rot: L\n");
        break;
      default:
        assert(false);  // Bad, bad, bad
        break;
    }
  })

  *next = *previous;

  next->history = previous;
  next->last_move = mv;

  assert(from_sq < ARR_SIZE && from_sq > 0);
  assert(next->board[from_sq] < (1 << PIECE_SIZE) &&
         next->board[from_sq] >= 0);
  assert(to_sq < ARR_SIZE && to_sq > 0);
  assert(next->board[to_sq] < (1 << PIECE_SIZE) &&
         next->board[to_sq] >= 0);

  next->key ^= zob_color;   // swap color to move

  piece_t from_piece = next->board[from_sq];
  piece_t to_piece = next->board[to_sq];

  if (to_sq != from_sq) {  // move, not rotation
    next->board[to_sq] = from_piece;  // swap from_piece and to_piece on board
    next->board[from_sq] = to_piece;

    // Hash key updates
    next->key ^= zob[from_sq][from_piece];  // remove from_piece from from_sq
    next->key ^= zob[to_sq][to_piece];  // remove to_piece from to_sq
    next->key ^= zob[to_sq][from_piece];  // place from_piece in to_sq
    next->key ^= zob[from_sq][to_piece];  // place to_piece in from_sq

    color_t from_piece_color = color_of(from_piece);
    color_t to_piece_color = color_of(to_piece);
    // Update King locations if necessary
    if (ptype_of(from_piece) == KING) {
      next->king_locs[from_piece_color] = to_sq;
    }
    if (ptype_of(to_piece) == KING) {
      next->king_locs[to_piece_color] = from_sq;
    }
    // Update Pawns location if neccessary
    if (ptype_of(from_piece) == PAWN) {
      for (int i = 0; i < PAWNS_COUNT; i++) {
        if (next->pawns_locs[from_piece_color][i] == from_sq) {
          next->pawns_locs[from_piece_color][i] = to_sq;
          break;
        }
      }
    }
    if (ptype_of(to_piece) == PAWN) {
      for (int i = 0; i < PAWNS_COUNT; i++) {
        if (next->pawns_locs[to_piece_color][i] == to_sq) {
          next->pawns_locs[to_piece_color][i] = from_sq;
          break;
        }
      }
    }

    assert(ptype_of(from_piece) == PAWN || ptype_of(from_piece) == KING);
    assert(ptype_of(to_piece) != INVALID);
    if (ptype_of(to_piece) == EMPTY) {
      rnk_t from_r = rnk_of(from_sq);
      rnk_t to_r = rnk_of(to_sq);
      fil_t from_f = fil_of(from_sq);
      fil_t to_f = fil_of(to_sq);
      next->bit_ranks[from_r] &= ~(1 << (BITS_PER_VECTOR - from_f - 1)); // set from_r'th column's from_f'th bit to 0. bitmask needed is all ones except f'th bit.
      next->bit_files[from_f] &= ~(1 << (BITS_PER_VECTOR - from_r - 1)); // set from_f'th row's from_r'th bit to 0
      next->bit_ranks[to_r] |= 1 << (BITS_PER_VECTOR - to_f - 1); // set to_r'th column's to_f'th bit to 1 
      next->bit_files[to_f] |= 1 << (BITS_PER_VECTOR - to_r - 1); // set to_f'th row's to_r'th bit to 1
    }
    //std::cout<<"\nChecking after piece move.";
    //check_bit_row_and_column(next);

    // Up
  } else {  // rotation

    // remove from_piece from from_sq in hash
    next->key ^= zob[from_sq][from_piece];
    set_ori(&from_piece, rot + orientation_of(from_piece));  // rotate from_piece
    next->board[from_sq] = from_piece;  // place rotated piece on board
    next->key ^= zob[from_sq][from_piece];              // ... and in hash
    //std::cout<<"\nChecking after piece rotation.";
    //check_bit_row_and_column(next);
  }

  // Increment ply
  next->ply++;

  assert(next->key == compute_zob_key(next));

  WHEN_DEBUG_VERBOSE({
    fprintf(stderr, "After:\n");
    display(next);
  })
}

// returns square of piece to be removed from board or 0
square_t fire(position_t *p) {
  color_t fctm = (color_to_move_of(p) == WHITE) ? BLACK : WHITE;
  square_t sq = p->king_locs[fctm];
  rnk_t r = rnk_of(sq);
  fil_t f = fil_of(sq);
  int bdir = orientation_of(p->board[sq]); // bdir: {0,1,2,3} -> {+1 sq, +12 sq, -1 sq, -12 sq}
  // +1, -1 => change in file
  // +12, -12 => corresponding change in rank
  uint16_t fire_range;
  int shift;
  int hit_pos;
  piece_t hit_piece;
  while(true) {
    switch(bdir) {
      case 0: // laser moves through increasing rank, constant file
        //std::cout<<"\n\nFiring through constant file: "<<(int(f))<<". Increasing rank starting at: "<<(int(r))<<". Start square: "<<sq;
        fire_range = p->bit_files[f];
        shift = r + 1; // left shift. find LS1.
        assert(shift >= 1 && shift <= 10);
        fire_range <<= shift;
        if (fire_range == 0) {
          //std::cout<<"\nNo piece in range.";
          return 0;
        }
        hit_pos = __builtin_clz(fire_range) - 15;
        // hit_pos = 1;
        // while (!(fire_range & 32768)) {
        //   hit_pos++;
        //   fire_range <<= 1;
        // }
        // assert(hit_pos == hit_pos2);
        assert(hit_pos >= 0 && hit_pos < BOARD_WIDTH);
        r += hit_pos;
        sq += hit_pos;
        //std::cout<<"\nLaser moves "<<hit_pos<<" steps and hits square: "<<sq;
        assert(sq >= 0);
        assert(sq < ARR_SIZE);
        assert(square_of(f,r) == sq);
        break;
      case 1: // laser moves through increasing file, constant rank
        //std::cout<<"\n\nFiring through increasing file starting at: "<<(int(f))<<". Constant rank: "<<(int(r))<<". Start square: "<<sq;
        fire_range = p->bit_ranks[r];
        shift = f + 1; // range: {1,...,10}. left shift. find MS1.
        assert(shift >= 1 && shift <= 10);
        fire_range <<= shift;
        if (fire_range == 0) {
          //std::cout<<"\nNo piece in range.";
          return 0;
        }
        hit_pos = __builtin_clz(fire_range) - 15;
        // hit_pos = 1;
        // while (!(fire_range & 32768)) {
        //   hit_pos++;
        //   fire_range <<= 1;
        // }
        // hit_pos now gives the difference in squares between previous piece and next hit piece
        assert(hit_pos >= 0 && hit_pos < BOARD_WIDTH);
        f += hit_pos;
        sq += ARR_WIDTH * hit_pos;
        //std::cout<<"\nLaser moves "<<hit_pos<<" steps and hits square: "<<sq;
        assert(sq >= 0);
        assert(sq < ARR_SIZE);
        assert(square_of(f,r) == sq);
        break;
      case 2: // laser moves through decreasing rank, constant file
        //std::cout<<"\n\nFiring through constant file: "<<(int(f))<<". Decreasing rank starting at: "<<(int(r))<<". Start square: "<<sq;
        fire_range = p->bit_files[f];
        shift = BOARD_WIDTH - r; // right shift. find MS1.
        assert(shift >= 1 && shift <= 10);
        fire_range >>= shift + (BITS_PER_VECTOR - BOARD_WIDTH);
        if (fire_range == 0) {
          //std::cout<<"\nNo piece in range.";
          return 0;
        }
        hit_pos = __builtin_ctz(fire_range) + 1;
        // hit_pos = 1;
        // while (!(fire_range & 1)) {
        //   hit_pos++;
        //   fire_range >>= 1;
        // }
        // assert(hit_pos2 == hit_pos);
        assert(hit_pos >= 0 && hit_pos < BOARD_WIDTH);
        r -= hit_pos;
        sq -= hit_pos;
        //std::cout<<"\nLaser moves "<<hit_pos<<" steps and hits square: "<<sq;
        assert(sq >= 0);
        assert(sq < ARR_SIZE);
        assert(square_of(f,r) == sq);
        break;
      case 3: // laser moves through decreasing file, constant rank
        //std::cout<<"\n\nFiring through decreasing file starting at: "<<(int(f))<<". Constant rank: "<<(int(r))<<". Start square: "<<sq;
        fire_range = p->bit_ranks[r];
        shift = BOARD_WIDTH - f; // range: {1,...,10}. right shift. find LS1.
        assert(shift >= 1 && shift <= 10);
        fire_range >>= shift + (BITS_PER_VECTOR - BOARD_WIDTH);
        if (fire_range == 0) {
          //std::cout<<"\nNo piece in range.";
          return 0;
        }
        hit_pos = __builtin_ctz(fire_range) + 1;
        // hit_pos = 1;
        // while (!(fire_range & 1)) {
        //   hit_pos++;
        //   fire_range >>= 1;
        // }
        assert(hit_pos >= 0 && hit_pos < BOARD_WIDTH);
        f -= hit_pos;
        sq -= ARR_WIDTH * hit_pos;
        //std::cout<<"\nLaser moves "<<hit_pos<<" steps and hits square: "<<sq;
        assert(sq >= 0);
        assert(sq < ARR_SIZE);
        assert(square_of(f,r) == sq);
        break;
      default:
        assert(false);
    }
    hit_piece = p->board[sq];
    if (ptype_of(hit_piece) == KING) {
      //std::cout<<"\nHit king. Returning.";
      return sq;
    }
    //std::cout<<"\nHit pawn with orientation: "<<orientation_of(hit_piece);
    //std::cout<<"\nBeam direction changed from "<<bdir;
    bdir = reflect_of(bdir, orientation_of(hit_piece));
    //std::cout<<" to "<<bdir;
    if (bdir < 0) {
      //std::cout<<"\nUnshielded pawn. Returning.";
      return sq;
    }
    //std::cout<<"\nLaser reflected. Continuing.";
  }
}

square_t fire_old(position_t *p) {
  color_t fctm = (color_to_move_of(p) == WHITE) ? BLACK : WHITE;
  square_t sq = p->king_locs[fctm];
  int bdir = orientation_of(p->board[sq]);

  assert(ptype_of(p->board[ p->king_locs[fctm] ]) == KING);

  while (true) {
    sq += beam_of(bdir);
    assert(sq < ARR_SIZE && sq >= 0);

    switch (ptype_of(p->board[sq])) {
     case EMPTY:  // empty square
      break;
     case PAWN:  // Pawn
      bdir = reflect_of(bdir, orientation_of(p->board[sq]));
      if (bdir < 0) {  // Hit back of Pawn
        return sq;
      }
      break;
     case KING:  // King
      return sq;  // sorry, game over my friend!
      break;
     case INVALID:  // Ran off edge of board
      return 0;
      break;
     default:  // Shouldna happen, man!
      assert(false);
      break;
    }
  }
}

square_t fire_test(position_t *p) {
  square_t fnew = fire(p);
  square_t fold = fire_old(p);
  //std::cout<<"\nOld fire returned: "<<fold<<". New fire: "<<fnew;
  assert(fnew == fold);
  return fnew;
}

// return 0 or victim piece or KO (== -1)
piece_t make_move(position_t *previous, position_t *next, move_t mv) {
  assert(mv != 0);

  low_level_make_move(previous, next, mv);

  //std::cout<<"\nMove "<<ptype_of(previous->board[from_square(mv)])<<" from "<<from_square(mv)<<" to "<<to_square(mv);
  square_t victim_sq = fire(next);

  WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
  WHEN_DEBUG_VERBOSE({
    if (victim_sq != 0) {
      square_to_str(victim_sq, buf);
      DEBUG_LOG(1, "Zapping piece on %s\n", buf);
    }
  })

  if (victim_sq == 0) {
    next->victim = 0;

    if (USE_KO &&  // Ko rule
        (next->key == (previous->key ^ zob_color) || next->key == previous->history->key))
      return KO;

  } else {  // we definitely hit something with laser
    next->victim = next->board[victim_sq];
    next->key ^= zob[victim_sq][next->victim];   // remove from board
    next->board[victim_sq] = 0;  
    next->key ^= zob[victim_sq][0];

    assert(ptype_of(next->victim) == PAWN || ptype_of(next->victim) == KING);
    rnk_t r = rnk_of(victim_sq);
    fil_t f = fil_of(victim_sq);
    //assert(r >= 0 && r < BOARD_WIDTH);
    //assert(f >= 0 && f < BOARD_WIDTH);

    next->bit_ranks[r] &= ~(1 << (BITS_PER_VECTOR - f - 1)); // set r'th column's f'th bit to 0. bitmask needed is all ones except f'th bit.
    next->bit_files[f] &= ~(1 << (BITS_PER_VECTOR - r - 1)); // set f'th row's r'th bit to 0
    
    color_t victim_color = color_of(next->victim);
    if (ptype_of(next->victim)) {
      for (int i = 0; i < PAWNS_COUNT; i++) {
        if (next->pawns_locs[victim_color][i] == victim_sq) {
          next->pawns_locs[victim_color][i] = 0;
          break;
        }
      }
    }
    //std::cout<<"\nChecking after piece death. Victim: "<<ptype_of(next->victim)<<" at location ("<<((int)f)<<", "<<((int)r)<<")";
    //check_bit_row_and_column(next);

    assert(next->key == compute_zob_key(next));

    WHEN_DEBUG_VERBOSE({
      square_to_str(victim_sq, buf);
      DEBUG_LOG(1, "Zapped piece on %s\n", buf);
    })
  }

  return next->victim;
}


// helper function for do_perft
// ply starting with 0
static uint64_t perft_search(position_t *p, int depth, int ply) {
  uint64_t node_count = 0;
  position_t np;
  sortable_move_t lst[MAX_NUM_MOVES];
  int num_moves;
  int i;

  if (depth == 0) {
    return 1;
  }

  num_moves = generate_all(p, lst, true);

  if (depth == 1) {
    return num_moves;
  }

  for (i = 0; i < num_moves; i++) {
    move_t mv = get_move(lst[i]);

    low_level_make_move(p, &np, mv);  // make the move baby!
    square_t victim_sq = fire(&np);  // the guy to disappear

    if (victim_sq != 0) {            // hit a piece
      ptype_t typ = ptype_of(np.board[victim_sq]);
      assert((typ != EMPTY) && (typ != INVALID));
      if (typ == KING) {  // do not expand further: hit a King
        node_count++;
        continue;
      }
      np.victim = np.board[victim_sq];
      np.key ^= zob[victim_sq][np.victim];   // remove from board
      np.board[victim_sq] = 0;
      np.key ^= zob[victim_sq][0];
    }

    uint64_t partialcount = perft_search(&np, depth-1, ply+1);
    node_count += partialcount;
  }

  return node_count;
}

// help to verify the move generator
void do_perft(position_t *gme, int depth, int ply) {
  fen_to_pos(gme, "");

  for (int d = 1; d <= depth; d++) {
    printf("perft %2d ", d);
    uint64_t j = perft_search(gme, d, 0);
    printf("%" PRIu64 "\n", j);
  }
}

void display(position_t *p) {
  char buf[MAX_CHARS_IN_MOVE];

  printf("\ninfo Ply: %d\n", p->ply);
  printf("info Color to move: %s\n", color_to_str(color_to_move_of(p)));

  square_to_str(p->king_locs[WHITE], buf);
  printf("info White King: %s, ", buf);
  square_to_str(p->king_locs[BLACK], buf);
  printf("info Black King: %s\n", buf);

  if (p->last_move != 0) {
    move_to_str(p->last_move, buf);
    printf("info Last move: %s\n", buf);
  } else {
    printf("info Last move: NULL\n");
  }

  for (rnk_t r = BOARD_WIDTH - 1; r >=0 ; --r) {
    printf("\ninfo %1d  ", r);
    for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
      square_t sq = square_of(f, r);

      if (ptype_of(p->board[sq]) == INVALID) {     // invalid square
        assert(false);                             // This is bad!
      }

      if (ptype_of(p->board[sq]) == EMPTY) {       // empty square
        printf(" --");
        continue;
      }

      int ori = orientation_of(p->board[sq]);  // orientation
      color_t c = color_of(p->board[sq]);

      if (ptype_of(p->board[sq]) == KING) {
        printf(" %2s", king_orientation_to_rep[c][ori]);
        continue;
      }

      if (ptype_of(p->board[sq]) == PAWN) {
        printf(" %2s", pawn_orientation_to_rep[c][ori]);
        continue;
      }
    }
  }

  printf("\n\ninfo    ");
  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    printf(" %c ", 'a'+f);
  }
  printf("\n\n");
}

