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
  int x = ((c >= a) && (c <= b)) || ((c <= a) && (c >= b));
  return x;
}

// assumes a <= b
inline bool betweenrange(int c, int a, int b) {
  return (c >= a && c <= b);
}

// PBETWEEN heuristic: Bonus for Pawn at (f, r) in rectangle defined by Kings at the corners
ev_score_t pbetween(position_t *p, fil_t f, rnk_t r) {
  bool is_between =
      between(f, fil_of(p->king_locs[WHITE]), fil_of(p->king_locs[BLACK])) &&
      between(r, rnk_of(p->king_locs[WHITE]), rnk_of(p->king_locs[BLACK]));
  return is_between ? PBETWEEN : 0;
}

inline ev_score_t kface(piece_t x, fil_t f, rnk_t r, fil_t otherf, rnk_t otherr) {
  assert(ptype_of(x) == KING);
  int delta_fil = otherf - f;
  int delta_rnk = otherr - r;
  int bonus;
  switch (orientation_of(x)) {
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
    assert(false);
  }

  return (bonus * KFACE) / (abs(delta_rnk) + abs(delta_fil));
}


// KFACE heuristic: bonus (or penalty) for King facing toward the other King
ev_score_t kface_old(position_t *p, fil_t f, rnk_t r) {
  square_t sq = square_of(f, r);
  piece_t x = p->board[sq];
  color_t c = color_of(x);
  square_t opp_sq = p->king_locs[opp_color(c)];
  int delta_fil = fil_of(opp_sq) - f;
  int delta_rnk = rnk_of(opp_sq) - r;
  int bonus;

  assert(x == p->board[sq]);
  switch (orientation_of(x)) {
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
inline ev_score_t kaggressive(fil_t f, rnk_t r, fil_t otherf, rnk_t otherr) {

  assert(f != otherf);
  assert(r != otherr);
  int bonus = 0;

  if (otherf >= f) {   // delta_fil >= 0 is equivalent to otherf >= f
    bonus = f + 1;
  } else {
    bonus = BOARD_WIDTH - f;
  }

  if (otherr >= r) {   // delta_rnk >= 0 is equivalent to otherr >= r
    bonus *= r + 1;
  } else {
    bonus *= BOARD_WIDTH - r;
  }

  return (KAGGRESSIVE * bonus) / (BOARD_WIDTH * BOARD_WIDTH);
}

ev_score_t kaggressive_old(position_t *p, fil_t f, rnk_t r) {
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
bool probe_for_mirror(position_t *p, square_t sq, int direction) {
  do {
    sq += beam_of(direction);
  } while (ptype_of(p->board[sq]) == EMPTY);

  return (ptype_of(p->board[sq]) == PAWN &&
          reflect_of(direction, orientation_of(p->board[sq])) >= 0);
}

// PMIRROR heuristic: penalty if opaque side of Pawn faces a mirror
// Penalty for one vulnerability = 2
// Penalty for two vulnerabilities = 3
ev_score_t pmirror(position_t *p, square_t sq) {
  piece_t x = p->board[sq];
  assert(ptype_of(x) == PAWN);
  int penalty = 0;

  for (int i = 1; i <= 2; ++i) {
    int direction = ORIENTATION_MASK & (i + orientation_of(x));
    if (probe_for_mirror(p, sq, direction)) {
      penalty += 2;
    }
  }
  penalty -= penalty >> 2;   // too clever by half!

  return penalty * PMIRROR;
}

ev_score_t pmirror_new(position_t *p, square_t sq) {
  piece_t x = p->board[sq];
  assert(ptype_of(x) == PAWN);
  int penalty = 0;
  int direction = ORIENTATION_MASK & (1 + orientation_of(x));  
  return 0;
}


// c is color-to-move
int king_vul(position_t *p, color_t c, square_t sq, king_orientation_t bdir) {
  // Returns bonus for attacking vulnerable squares
  // Returns -1 if reverse path is too dangerous

  square_t king_sq = p->king_locs[opp_color(c)];
  piece_t x = p->board[king_sq];
  assert(ptype_of(x) == KING);
  assert(color_of(x) != c);
  king_orientation_t k_ori = (king_orientation_t) orientation_of(x);

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
  bdir = (king_orientation_t) ((bdir - k_ori) & ORIENTATION_MASK);
  return 0;
}

void mark_laser_path(position_t *p, bool * laser_map, color_t c) {
  position_t np = *p;
  
  // Fire laser, recording in laser_map
  square_t sq = np.king_locs[c];
  int bdir = orientation_of(np.board[sq]);
  
  assert(ptype_of(np.board[sq]) == KING);
  laser_map[sq] = true;
  
  
  while (true) {
    sq += beam_of(bdir);
    laser_map[sq] = true;
    assert(sq < ARR_SIZE && sq >= 0);
    
    switch (ptype_of(p->board[sq])) {
      case EMPTY:  // empty square
        break;
      case PAWN:  // Pawn
        bdir = reflect_of(bdir, orientation_of(p->board[sq]));
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


// int mobility(position_t *p, color_t color) {
//   color_t c = opp_color(color);
//   bool laser_map[ARR_SIZE];

//   for (int i = 0; i < ARR_SIZE; ++i) {
//     laser_map[i] = 4;   // Invalid square
//   }

//   for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
//     for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
//       laser_map[square_of(f, r)] = 0;
//     }
//   }

//   mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

//   sortable_move_t lst[MAX_NUM_MOVES];
//   int save_ply = p->ply;
//   p->ply = c;  // fake out generate_all as to whose turn it is
//   int num_moves = generate_all(p, lst, true);
//   p->ply = save_ply;  // restore

//   for (int i = 0; i < num_moves; ++i) {
//     if ((laser_map[from_square(get_move(lst[i]))] & 1) != 1 &&
//         (laser_map[to_square(get_move(lst[i]))] & 1) != 1) {
//       // move can't affect path of laser
//       continue;
//     }
//     mark_laser_path(p, laser_map, c, 2);  // 2 = path of laser with move
//   }

//   // mobility = # safe squares around enemy king

//   square_t king_sq = p->king_locs[color];
//   assert(ptype_of(p->board[king_sq]) == KING);
//   assert(color_of(p->board[king_sq]) == color);

//   int mobility = 0;
//   if (laser_map[king_sq] == 0) {
//     mobility++;
//   }
//   for (int d = 0; d < 8; ++d) {
//     square_t sq = king_sq + dir_of(d);
//     if (laser_map[sq] == 0) {
//       mobility++;
//     }
//   }
//   return mobility;
// }

// int squares_attackable(position_t *p, color_t c) {
//   bool laser_map[ARR_SIZE];

//   for (int i = 0; i < ARR_SIZE; ++i)
//     laser_map[i] = 4;   // Invalid square

//   for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
//     for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
//       laser_map[square_of(f, r)] = 0;
//     }
//   }

//   mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

//   sortable_move_t lst[MAX_NUM_MOVES];
//   int save_ply = p->ply;
//   p->ply = c;  // fake out generate_all as to whose turn it is
//   int num_moves = generate_all(p, lst, true);
//   p->ply = save_ply;  // restore

//   for (int i = 0; i < num_moves; ++i) {
//     if ((laser_map[from_square(get_move(lst[i]))] & 1) != 1 &&
//         (laser_map[to_square(get_move(lst[i]))] & 1) != 1) {
//       // move can't affect path of laser
//       continue;
//     }
//     mark_laser_path(p, laser_map, c, 2);  // 2 = path of laser with move
//   }

//   int attackable = 0;
//   for (fil_t f = 0; f < BOARD_WIDTH; f++) {
//     for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
//       if (laser_map[square_of(f, r)] != 0) {
//         attackable++;
//       }
//     }
//   }
//   return attackable;
// }

int h_squares_attackable(position_t *p, color_t c) {
  float h_attackable = 0;
  square_t o_king_sq = p->king_locs[opp_color(c)];

  // Fire laser, recording in laser_map
  square_t sq = p->king_locs[c];
  int bdir = orientation_of(p->board[sq]);
  int beam = beam_of(bdir);
  assert(ptype_of(p->board[sq]) == KING);
  h_attackable += h_dist(sq, o_king_sq);  

  while (true) {
    assert(beam == beam_of(bdir));
    sq += beam;
    assert(sq < ARR_SIZE && sq >= 0);
    
    switch (ptype_of(p->board[sq])) {
      case EMPTY:  // empty square
        h_attackable += h_dist(sq, o_king_sq);  
        break;
      case PAWN:  // Pawn
        h_attackable += h_dist(sq, o_king_sq);  
        bdir = reflect_of(bdir, orientation_of(p->board[sq]));
        if (bdir < 0) {  // Hit back of Pawn
          return h_attackable;
        }
        beam = beam_of(bdir);
        break;
      case KING:  // King
        h_attackable += h_dist(sq, o_king_sq);  
        return h_attackable;  // sorry, game over my friend!
        break;
      case INVALID:  // Ran off edge of board
        return h_attackable;
        break;
      default:  // Shouldna happen, man!
        assert(false);
        break;
    }
  }
}

int h_squares_attackable_old(position_t *p, color_t c) {
  bool laser_map[ARR_SIZE];
  for (int i = 0; i < ARR_SIZE; i++) {
    laser_map[i] = false;
  }
  
  mark_laser_path(p, laser_map, c);  // 1 = path of laser with no moves
  
  square_t o_king_sq = p->king_locs[opp_color(c)];
  assert(ptype_of(p->board[o_king_sq]) == KING);
  assert(color_of(p->board[o_king_sq]) != c);
  
  float h_attackable = 0;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t sq = square_of(f, r);
      if (laser_map[sq]) {
          h_attackable += h_dist(sq, o_king_sq);
      }
    }
  }
  return h_attackable;  
}

int h_squares_attackable_test(position_t *p, color_t c) {
  int hnew = h_squares_attackable(p, c);
  int hold = h_squares_attackable_old(p, c);
  if (hnew != hold) {
    std::cout<<"\nh_squares_attackable_old: "<<hold<<". h_squares_attackable_new: "<<hnew;
  }
  return hnew;
}

// Unoptimized eval function for testing purpuses
score_t unoptimized_eval(position_t *p, bool verbose) {
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
          bonus = kface_old(p, f, r);
          if (verbose) {
            printf("KFACE bonus %d for %s King on %s\n", bonus,
                   color_to_str(c), buf);
          }
          score[c] += bonus;

          // KAGGRESSIVE heuristic
          bonus = kaggressive_old(p, f, r);
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

// Static evaluation.  Returns score
score_t eval(position_t *p, bool verbose) {
  ev_score_t score[2] = { 0, 0 };
  //  int corner[2][2] = { {INF, INF}, {INF, INF} };
  ev_score_t bonus;
  char buf[MAX_CHARS_IN_MOVE];
  bool is_between;
  fil_t king_fil[2] = { fil_of(p->king_locs[0]), fil_of(p->king_locs[1]) }; // respects ordering of WHITE and BLACK
  rnk_t king_rnk[2] = { rnk_of(p->king_locs[0]), rnk_of(p->king_locs[1]) }; // respects ordering of WHITE and BLACK

  // King heuristics
  bonus = kface(p->board[p->king_locs[BLACK]], king_fil[BLACK], king_rnk[BLACK], king_fil[WHITE], king_rnk[WHITE]);
  assert(bonus == kface_old(p, king_fil[BLACK], king_rnk[BLACK]));
  score[BLACK] += bonus;
  bonus = kface(p->board[p->king_locs[WHITE]], king_fil[WHITE], king_rnk[WHITE], king_fil[BLACK], king_rnk[BLACK]);
  assert(bonus == kface_old(p, king_fil[WHITE], king_rnk[WHITE]));
  score[WHITE] += bonus;
  // KAGGRESSIVE heuristic
  bonus = kaggressive(king_fil[BLACK], king_rnk[BLACK], king_fil[WHITE], king_rnk[WHITE]);
  assert(bonus == kaggressive_old(p, king_fil[BLACK], king_rnk[BLACK]));
  score[BLACK] += bonus;
  bonus = kaggressive(king_fil[WHITE], king_rnk[WHITE], king_fil[BLACK], king_rnk[BLACK]);
  assert(bonus == kaggressive_old(p, king_fil[WHITE], king_rnk[WHITE]));
  score[WHITE] += bonus;

  if (king_rnk[0] > king_rnk[1]) { // sort king_rnk. won't respect ordering of WHITE and BLACK anymore
    rnk_t tmp = king_rnk[0];
    king_rnk[0] = king_rnk[1];
    king_rnk[1] = tmp;
  }
  if (king_fil[0] > king_fil[1]) { // sort king_fil. won't respect ordering of WHITE and BLACK anymore
    fil_t tmp = king_fil[0];
    king_fil[0] = king_fil[1];
    king_fil[1] = tmp;
  }
  int i;
  square_t sq;
  for (int c = 0; c < 2; c++) { // consider unrolling this loop or using pointer access
    i = 0;
    while (sq = p->pawns_locs[c][i]) {
      // MATERIAL heuristic: Bonus for each Pawn
      bonus = PAWN_EV_VALUE;
      score[c] += bonus;
      // PBETWEEN heuristic
      if (betweenrange(fil_of(sq), king_fil[0], king_fil[1]) && 
          betweenrange(rnk_of(sq), king_rnk[0], king_rnk[1])) {
        bonus = PBETWEEN;
        score[c] += bonus;
        // PMIRROR penalty
        //score[c] += pmirror(p, sq);
      }
      i++;
    }
  }
  
  // Make sure that the squares in the pawns_locs are unique.
  // for (int i = 0; i < 2 * PAWNS_COUNT; i++) {
  //   square_t sq = *(*(p->pawns_locs) + i);
  //   for (int j = i+1; j < 2 * PAWNS_COUNT; j++) {
  //     square_t sq2 = *(*(p->pawns_locs) + j);
  //     if (sq) {
  //       assert( sq != sq2);
  //     }
  //   }
  // }

  ev_score_t w_hattackable = HATTACK * h_squares_attackable(p, WHITE);
  score[WHITE] += w_hattackable;

  ev_score_t b_hattackable = HATTACK * h_squares_attackable(p, BLACK);
  score[BLACK] += b_hattackable;

  // score from WHITE point of view
  ev_score_t tot = score[WHITE] - score[BLACK];

  if (RANDOMIZE) {
    ev_score_t  z = rand() % (RANDOMIZE*2+1);
    tot = tot + z - RANDOMIZE;
  }

  if (color_to_move_of(p) == BLACK) {
    tot = -tot;
  }
  assert(unoptimized_eval(p, verbose) == tot / EV_SCORE_RATIO);
  return tot / EV_SCORE_RATIO;
}
