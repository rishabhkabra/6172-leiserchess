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
#include <stdlib.h>
#include <stdio.h>

#include "eval.h"

//----------------------------------------------------------------------
// Evaluation

typedef int32_t ev_score_t;  // Static evaluator uses "hi res" values

int RANDOMIZE;

int HATTACK;
int ATTACK;
int PMIRROR;
int PBETWEEN;
int KFACE;
int KAGGRESSIVE;

// Heuristics for static evaluation

// returns true if c lies on or between a and b, which are not ordered
bool between(int c, int a, int b) {
  bool x = ((c >= a) && (c <= b)) || ((c <= a) && (c >= b));
  return x;
}

// PBETWEEN heuristic: Bonus for Pawn at (f, r) in rectangle defined by Kings at the corners
ev_score_t pbetween(position_t *p, fil_t f, rnk_t r) {
  bool is_between =
      between(f, fil_of(p->king_locs[WHITE]), fil_of(p->king_locs[BLACK])) &&
      between(r, rnk_of(p->king_locs[WHITE]), rnk_of(p->king_locs[BLACK]));
  return is_between ? PBETWEEN : 0;
}


// KFACE heuristic: bonus (or penalty) for King facing toward the other King
ev_score_t kface(position_t *p, fil_t f, rnk_t r) {
  square_t sq = square_of(f, r);
  piece_t x = p->board[sq];
  color_t c = color_of(x);
  square_t opp_sq = p->king_locs[opp_color(c)];
  int delta_fil = fil_of(opp_sq) - f;
  int delta_rnk = rnk_of(opp_sq) - r;
  int bonus;

  switch (ori_of(p->board[sq])) {
   case NN:
    bonus = delta_rnk;
    break;

   case EE:
    bonus = delta_fil;
    break;

   case SS:
    bonus = -delta_rnk;
    break;

   case WW:
    bonus = -delta_fil;
    break;

   default:
    bonus = 0;
    printf("Illegal King orientation\n");
    assert(false);
  }

  return (bonus * KFACE) / (abs(delta_rnk) + abs(delta_fil));
}

// KAGGRESSIVE heuristic: bonus for King with more space to back
ev_score_t kaggressive(position_t *p, fil_t f, rnk_t r) {
  square_t sq = square_of(f, r);
  piece_t x = p->board[sq];
  color_t c = color_of(x);
  assert(ptype_of(x) == KING);

  square_t opp_sq = p->king_locs[opp_color(c)];
  fil_t of = fil_of(opp_sq);
  rnk_t _or = (rnk_t) rnk_of(opp_sq);

  int delta_fil = of - f;
  int delta_rnk = _or - r;

  int bonus = 0;

  if (delta_fil >= 0 && delta_rnk >= 0) {
    bonus = (f + 1) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk >= 0) {
    bonus = (BOARD_WIDTH - f) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk <= 0) {
    bonus = (BOARD_WIDTH - f) * (BOARD_WIDTH - r);
  } else if (delta_fil >= 0 && delta_rnk <= 0) {
    bonus = (f + 1) * (BOARD_WIDTH - r);
  }

  return (KAGGRESSIVE * bonus) / (BOARD_WIDTH * BOARD_WIDTH);
}

// Aux routine to look in a given direction and see whether the first piece
// hit is a mirror (true) or not.
inline bool probe_for_mirror(position_t *p, square_t sq, int direction) {
  do {
    sq += beam_of(direction);
  } while (ptype_of(p->board[sq]) == EMPTY);

  return (ptype_of(p->board[sq]) == PAWN &&
          reflect_of(direction, ori_of(p->board[sq])) >= 0);
}

// PMIRROR heuristic: penalty if opaque side of Pawn faces a mirror
// Penalty for one vulnerability = 2
// Penalty for two vulnerabilities = 3
ev_score_t pmirror(position_t *p, fil_t f, rnk_t r) {
  square_t sq = square_of(f, r);
  piece_t x = p->board[sq];

  assert(ptype_of(x) == PAWN);

  int penalty = 0;

  for (int i = 1; i <= 2; ++i) {
    int direction = ORI_MASK & (i + ori_of(x));
    if (probe_for_mirror(p, sq, direction)) {
      penalty += 2;
    }
  }
  penalty -= penalty >> 2;   // too clever by half!

  return penalty * PMIRROR;
}

// c is color-to-move
int king_vul(position_t *p, color_t c, square_t sq, king_ori_t bdir) {
  // Returns bonus for attacking vulnerable squares
  // Returns -1 if reverse path is too dangerous

  square_t king_sq = p->king_locs[opp_color(c)];
  piece_t x = p->board[king_sq];
  assert(ptype_of(x) == KING);
  assert(color_of(x) != c);
  king_ori_t k_ori = (king_ori_t) ori_of(x);

  fil_t f = fil_of(sq);
  rnk_t r = rnk_of(sq);

  // Normalize to king_sq at (0,0) facing north
  rnk_t tmpr;
  fil_t tmpf;
  switch (k_ori) {
  case NN:
    f = f - fil_of(king_sq);
    r = r - rnk_of(king_sq);
    break;
  case EE:
    tmpr = r - rnk_of(king_sq);
    r = f - fil_of(king_sq);
    f = tmpr;
    break;
  case SS:
    f = fil_of(king_sq) - f;
    r = rnk_of(king_sq) - r;
    break;
  case WW:
    tmpf = rnk_of(king_sq) - r;
    r = fil_of(king_sq) - f;
    f = tmpf;
    break;
  }
  bdir = (king_ori_t) ((bdir - k_ori) & ORI_MASK);
  return 0;
}

void mark_laser_path(position_t *p, char *laser_map, color_t c,
                     char mark_mask) {
  position_t np = *p;

  // Fire laser, recording in laser_map
  square_t sq = np.king_locs[c];
  int bdir = ori_of(np.board[sq]);

  assert(ptype_of(np.board[sq]) == KING);
  laser_map[sq] |= mark_mask;

  while (true) {
    sq += beam_of(bdir);
    laser_map[sq] |= mark_mask;
    assert(sq < ARR_SIZE && sq >= 0);

    switch (ptype_of(p->board[sq])) {
     case EMPTY:  // empty square
      break;
     case PAWN:  // Pawn
      bdir = reflect_of(bdir, ori_of(p->board[sq]));
      if (bdir < 0) {  // Hit back of Pawn
        return;
      }
      break;
     case KING:  // King
      return;  // sorry, game over my friend!
      break;
     case INVALID:  // Ran off edge of board
      return;
      break;
     default:  // Shouldna happen, man!
      assert(false);
      break;
    }
  }
}

int mobility(position_t *p, color_t color) {
  color_t c = opp_color(color);
  char laser_map[ARR_SIZE];

  for (int i = 0; i < ARR_SIZE; ++i) {
    laser_map[i] = 4;   // Invalid square
  }

  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      laser_map[square_of(f, r)] = 0;
    }
  }

  mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

  sortable_move_t lst[MAX_NUM_MOVES];
  int save_ply = p->ply;
  p->ply = c;  // fake out generate_all as to whose turn it is
  int num_moves = generate_all(p, lst, true);
  p->ply = save_ply;  // restore

  for (int i = 0; i < num_moves; ++i) {
    if ((laser_map[from_square(get_move(lst[i]))] & 1) != 1 &&
        (laser_map[to_square(get_move(lst[i]))] & 1) != 1) {
      // move can't affect path of laser
      continue;
    }
    mark_laser_path(p, laser_map, c, 2);  // 2 = path of laser with move
  }

  // mobility = # safe squares around enemy king

  square_t king_sq = p->king_locs[color];
  assert(ptype_of(p->board[king_sq]) == KING);
  assert(color_of(p->board[king_sq]) == color);

  int mobility = 0;
  if (laser_map[king_sq] == 0) {
    mobility++;
  }
  for (int d = 0; d < 8; ++d) {
    square_t sq = king_sq + dir_of(d);
    if (laser_map[sq] == 0) {
      mobility++;
    }
  }
  return mobility;
}

int squares_attackable(position_t *p, color_t c) {
  char laser_map[ARR_SIZE];

  for (int i = 0; i < ARR_SIZE; ++i)
    laser_map[i] = 4;   // Invalid square

  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      laser_map[square_of(f, r)] = 0;
    }
  }

  mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

  sortable_move_t lst[MAX_NUM_MOVES];
  int save_ply = p->ply;
  p->ply = c;  // fake out generate_all as to whose turn it is
  int num_moves = generate_all(p, lst, true);
  p->ply = save_ply;  // restore

  for (int i = 0; i < num_moves; ++i) {
    if ((laser_map[from_square(get_move(lst[i]))] & 1) != 1 &&
        (laser_map[to_square(get_move(lst[i]))] & 1) != 1) {
      // move can't affect path of laser
      continue;
    }
    mark_laser_path(p, laser_map, c, 2);  // 2 = path of laser with move
  }

  int attackable = 0;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      if (laser_map[square_of(f, r)] != 0) {
        attackable++;
      }
    }
  }
  return attackable;
}


// Harmonic-ish distance: 1/(|dx|+1) + 1/(|dy|+1)
float h_dist(square_t a, square_t b) {
  //  printf("a = %d, FIL(a) = %d, RNK(a) = %d\n", a, FIL(a), RNK(a));
  //  printf("b = %d, FIL(b) = %d, RNK(b) = %d\n", b, FIL(b), RNK(b));
  int delta_fil = abs(fil_of(a) - fil_of(b));
  int delta_rnk = abs(rnk_of(a) - rnk_of(b));
  float x = (1.0 / (delta_fil + 1)) + (1.0 / (delta_rnk + 1));
  //  printf("max_dist = %d\n\n", x);
  return x;
}

int h_squares_attackable(position_t *p, color_t c) {
  char laser_map[ARR_SIZE];

  for (int i = 0; i < ARR_SIZE; ++i) {
    laser_map[i] = 4;   // Invalid square
  }

  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      laser_map[square_of(f, r)] = 0;
    }
  }

  mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

  sortable_move_t lst[MAX_NUM_MOVES];
  int save_ply = p->ply;
  p->ply = c;  // fake out generate_all as to whose turn it is
  int num_moves = generate_all(p, lst, true);
  p->ply = save_ply;  // restore

  for (int i = 0; i < num_moves; ++i) {
    if ((laser_map[from_square(get_move(lst[i]))] & 1) != 1 &&
        (laser_map[to_square(get_move(lst[i]))] & 1) != 1) {
      // move can't affect path of laser
      continue;
    }
    mark_laser_path(p, laser_map, c, 2);  // 2 = path of laser with move
  }

  square_t o_king_sq = p->king_locs[opp_color(c)];
  assert(ptype_of(p->board[o_king_sq]) == KING);
  assert(color_of(p->board[o_king_sq]) != c);

  float h_attackable = 0;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t sq = square_of(f, r);
      if (laser_map[sq] != 0) {
        h_attackable += h_dist(sq, o_king_sq);
      }
    }
  }
  return h_attackable;
}

// Static evaluation.  Returns score
score_t eval(position_t *p, bool verbose) {
  // verbose = true: print out components of score
  ev_score_t score[2] = { 0, 0 };
  //  int corner[2][2] = { {INF, INF}, {INF, INF} };
  ev_score_t bonus;
  char buf[MAX_CHARS_IN_MOVE];

  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t sq = square_of(f, r);
      piece_t x = p->board[sq];
      color_t c = color_of(x);
      if (verbose) {
        square_to_str(sq, buf);
      }

      switch (ptype_of(x)) {
        case EMPTY:
          break;
        case PAWN:
          // MATERIAL heuristic: Bonus for each Pawn
          bonus = PAWN_EV_VALUE;
          if (verbose) {
            printf("MATERIAL bonus %d for %s Pawn on %s\n", bonus, color_to_str(c), buf);
          }
          score[c] += bonus;

          // PBETWEEN heuristic
          bonus = pbetween(p, f, r);
          if (verbose) {
            printf("PBETWEEN bonus %d for %s Pawn on %s\n", bonus, color_to_str(c), buf);
          }
          score[c] += bonus;
          break;

        case KING:
          // KFACE heuristic
          bonus = kface(p, f, r);
          if (verbose) {
            printf("KFACE bonus %d for %s King on %s\n", bonus,
                   color_to_str(c), buf);
          }
          score[c] += bonus;

          // KAGGRESSIVE heuristic
          bonus = kaggressive(p, f, r);
          if (verbose) {
            printf("KAGGRESSIVE bonus %d for %s King on %s\n", bonus, color_to_str(c), buf);
          }
          score[c] += bonus;
          break;
        case INVALID:
          break;
        default:
          assert(false);   // No way, Jose!
      }
    }
  }

  ev_score_t w_hattackable = HATTACK * h_squares_attackable(p, WHITE);
  score[WHITE] += w_hattackable;
  if (verbose) {
    printf("HATTACK bonus %d for White\n", w_hattackable);
  }
  ev_score_t b_hattackable = HATTACK * h_squares_attackable(p, BLACK);
  score[BLACK] += b_hattackable;
  if (verbose) {
    printf("HATTACK bonus %d for Black\n", b_hattackable);
  }

  // score from WHITE point of view
  ev_score_t tot = score[WHITE] - score[BLACK];

  if (RANDOMIZE) {
    ev_score_t  z = rand() % (RANDOMIZE*2+1);
    tot = tot + z - RANDOMIZE;
  }

  if (color_to_move_of(p) == BLACK) {
    tot = -tot;
  }

  return tot / EV_SCORE_RATIO;
}
