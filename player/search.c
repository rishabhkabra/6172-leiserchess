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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "eval.h"
#include "search.h"
#include "tt.h"
#include "util.h"

//----------------------------------------------------------------------
// Sort stuff

typedef uint32_t sort_key_t;
static const uint64_t SORT_MASK = (1ULL << 32) - 1;
static const int SORT_SHIFT = 32;

// value of a draw
int DRAW;

// POSITIONAL WEIGHTS evaluation terms
int HMB;       // having the move bonus

// Late-move reduction
int LMR_R1;    // Look at this number of moves full width before reducing 1 ply
int LMR_R2;    // After this number of moves reduce 2 ply

int USE_NMM;
int TRACE_MOVES;   // Print moves
int DETECT_DRAWS;  // Detect draws by repetition

// do not set more than 5 ply
int FUT_DEPTH;     // set to zero for no futilty


static move_t killer[MAX_PLY_IN_SEARCH][4];   // up to 4 killers


sort_key_t sort_key(sortable_move_t mv) {
  return (sort_key_t) ((mv >> SORT_SHIFT) & SORT_MASK);
}

void set_sort_key(sortable_move_t *mv, sort_key_t key) {
  // sort keys must not exceed SORT_MASK
  //  assert ((0 <= key) && (key <= SORT_MASK));
  *mv = ((((uint64_t) key) & SORT_MASK) << SORT_SHIFT) |
        (*mv & ~(SORT_MASK << SORT_SHIFT));
  return;
}

move_t get_move(sortable_move_t sortable_mv) {
  return (move_t) (sortable_mv & MOVE_MASK);
}


// note: these need to be tuned but this should be pretty conservative
//       probably we would only use 3 or 4 of these values at most
// score_t fmarg[10] = { 0,  50, 100, 250, 450, 700, 1000, 1500, 2000, 3000 };
static score_t fmarg[10] = {
  0, PAWN_VALUE / 2, PAWN_VALUE, (PAWN_VALUE * 5) / 2, (PAWN_VALUE * 9) / 2,
  PAWN_VALUE * 7, PAWN_VALUE * 10, PAWN_VALUE * 15, PAWN_VALUE * 20,
  PAWN_VALUE * 30
};

void init_killer() {
  memset(killer, 0, sizeof(killer));  // clear any killer info
}


#define ABORT_CHECK_PERIOD 0xfff

// tic counter for how often we should check for abort
static int     tics = 0;
static double  sstart;    // start time of a search in milliseconds
static double  timeout;   // time elapsed before abort
static bool    abortf = false;  // abort flag for search

void init_abort_timer(double goal_time) {
  sstart = milliseconds();
  // don't go over any more than 3 times the goal
  timeout = sstart + goal_time * 3.0;
}

double elapsed_time() {
  return milliseconds() - sstart;
}

bool should_abort() {
  return abortf;
}

void reset_abort() {
  abortf = false;
}

void init_tics() {
  tics = 0;
}

// --------------------------
// Detect repetition
// --------------------------
static bool is_repeated(position_t *p, score_t *score, int ply) {
  if (!DETECT_DRAWS) {
    return false;   // no draw detected
  }

  position_t *x = p->history;
  uint64_t cur = p->key;

  while (true) {
    if (x->victim) {
      break;  // cannot be a repetition
    }
    x = x->history;
    if (x->victim) {
      break;  // cannot be a repetition
    }
    if (x->key == cur) {               // is a repetition
      if (ply & 1) {
        *score = -DRAW;
      } else {
        *score = DRAW;
      }
      return true;
    }
    x = x->history;
  }
  return false;
}

//      best_move_history[color_t][piece_t][square_t][orientation]
static int best_move_history[2][6][ARR_SIZE][NUM_ORI];

void init_best_move_history() {
  memset(best_move_history, 0, sizeof(best_move_history));
}

static void update_best_move_history(position_t *p, int index_of_best,
                                     sortable_move_t *lst, int count) {
  int ctm = color_to_move_of(p);

  for (int i = 0; i < count; i++) {
    move_t   mv  = get_move(lst[i]);
    ptype_t  pce = ptype_of(mv);
    rot_t    ro  = rot_of(mv);   // rotation
    square_t fs  = from_square(mv);
    int      ot  = ORI_MASK & (ori_of(p->board[fs]) + ro);
    square_t ts  = to_square(mv);

    int  s = best_move_history[ctm][pce][ts][ot];

    if (index_of_best == i) {
      s = s + 11200;     // number will never exceed 1017
    }
    s = s * 0.90;

    assert(s < 102000);  // or else sorting will fail

    best_move_history[ctm][pce][ts][ot] = s;
  }
}

void getPV(move_t *pv, char *buf) {
  buf[0] = 0;

  for (int i = 0; i < (MAX_PLY_IN_SEARCH - 1) && pv[i] != 0; i++) {
    char a[MAX_CHARS_IN_MOVE];
    move_to_str(pv[i], a);
    if (i != 0) {
      strcat(buf, " ");
    }
    strcat(buf, a);
  }
  return;
}

// Main search routines and helper functions
typedef enum searchType {  // different types of search
  SEARCH_PV,
  SEARCH_NON_PV,
} searchType_t;

static void print_move_info(move_t mv, int ply) {
  char buf[MAX_CHARS_IN_MOVE];
  move_to_str(mv, buf);
  printf("info");
  for (int i = 0; i < ply; ++i) {
    printf(" ----");
  }
  printf(" %s\n", buf);
}

// check the victim piece returned by the move to determine if it's a
// game-over situation
// if so, also calculate the score depending on the pov (which player's
// point of view)
static bool is_game_over(piece_t victim, score_t *score, int pov, int ply) {
  if (ptype_of(victim) == KING) {
    if (color_of(victim) == WHITE) {
      *score = -WIN * pov;
    } else {
      *score = WIN * pov;
    }
    if (*score < 0) {
      *score += ply;
    } else {
      *score -= ply;
    }
    return true;
  }
  return false;
}

static score_t scout_search(position_t *p, score_t beta, int depth,
                            int ply, int reduction, move_t *pv, uint64_t *node_count) {
  if (reduction > 0) {
    // We first perform a reduced depth search.
    int score = scout_search(p, beta, depth - reduction, ply, 0, pv, node_count);

    // -(parentBeta-1) = beta --> parentBeta = -beta+1
    int parentBeta = -beta + 1;
    int parentScore = -score;

    // No need to search to full depth, return this score.
    if (parentScore < parentBeta) {
      return score;
    }

    if (abortf) {
      return 0;
    }
  }

  pv[0] = 0;

  // check whether we should abort
  tics++;
  if ((tics & ABORT_CHECK_PERIOD) == 0) {
    if (milliseconds() >= timeout) {
      abortf = true;
      return 0;
    }
  }

  // get transposition table record if available
  ttRec_t *rec = tt_hashtable_get(p->key);
  int hash_table_move = 0;
  if (rec) {
    if (tt_is_usable(rec, depth, beta)) {
      return tt_adjust_score_from_hashtable(rec, ply);
    }
    hash_table_move = tt_move_of(rec);
  }

  score_t best_score = -INF;
  score_t sps = eval(p, false) + HMB;  // stand pat (having-the-move) bonus
  bool quiescence = (depth <= 0);      // are we in quiescence?

  if (quiescence) {
    best_score = sps;
    if (best_score >= beta) {
      return best_score;
    }
  }

  // margin based forward pruning
  if (USE_NMM) {
    if (depth <= 2) {
      if (depth == 1 && sps >= beta + 3 * PAWN_VALUE) {
        return beta;
      }
      if (depth == 2 && sps >= beta + 5 * PAWN_VALUE) {
        return beta;
      }
    }
  }

  // futility pruning
  if (depth <= FUT_DEPTH && depth > 0) {
    if (sps + fmarg[depth] < beta) {
      // treat this ply as a quiescence ply, look only at captures
      quiescence = true;
      best_score = sps;
    }
  }

  position_t np;  // next position
  // hopefully, more than we will need
  sortable_move_t move_list[MAX_NUM_MOVES];
  // number of moves in list
  int num_of_moves = generate_all(p, move_list, false);

  color_t fctm = color_to_move_of(p);
  int pov = 1 - fctm*2;      // point of view = 1 for white, -1 for black
  move_t killer_a = killer[ply][0];
  move_t killer_b = killer[ply][1];

  // sort special moves to the front
  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    move_t mv = get_move(move_list[mv_index]);
    if (mv == hash_table_move) {
      set_sort_key(&move_list[mv_index], SORT_MASK);
      // move_list[mv_index] |= SORT_MASK << SORT_SHIFT;
    } else if (mv == killer_a) {
      set_sort_key(&move_list[mv_index], SORT_MASK - 1);
      // move_list[mv_index] |= (SORT_MASK - 1) << SORT_SHIFT;
    } else if (mv == killer_b) {
      set_sort_key(&move_list[mv_index], SORT_MASK - 2);
      // move_list[mv_index] |= (SORT_MASK - 2) << SORT_SHIFT;
    } else {
      ptype_t  pce = ptype_of(mv);
      rot_t    ro  = rot_of(mv);   // rotation
      square_t fs  = from_square(mv);
      int      ot  = ORI_MASK & (ori_of(p->board[fs]) + ro);
      square_t ts  = to_square(mv);
      set_sort_key(&move_list[mv_index], best_move_history[fctm][pce][ts][ot]);
    }
  }

  move_t subpv[MAX_PLY_IN_SEARCH];
  score_t score;

  bool sortme = true;
  int legal_move_count = 0;
  int mv_index;  // used outside of the loop
  int best_move_index = 0;   // index of best move found

  for (mv_index = 0; mv_index < num_of_moves; mv_index++) {
    subpv[0] = 0;

    // on the fly sorting
    if (sortme) {
      for (int j = mv_index + 1; j < num_of_moves; j++) {
        if (move_list[j] > move_list[mv_index]) {
          sortable_move_t tmp = move_list[j];
          move_list[j] = move_list[mv_index];
          move_list[mv_index] = tmp;
        }
      }
      if (sort_key(move_list[mv_index]) == 0) {
        sortme = false;
      }
    }

    move_t mv = get_move(move_list[mv_index]);
    if (TRACE_MOVES) {
      print_move_info(mv, ply);
    }

    int ext = 0;           // extensions
    bool blunder = false;  // shoot our own piece

    (*node_count)++;
    piece_t victim = make_move(p, &np, mv);  // make the move baby! returns 0 or victim piece or KO (== -1)
    if (victim == KO) {
      continue;
    }

    if (is_game_over(victim, &score, pov, ply)) {
      // Break out of loop.
      goto scored;
    }

    if (victim == 0 && quiescence) {
      continue;   // ignore noncapture moves in quiescence
    }
    if (color_of(np.victim) == fctm) {
      blunder = true;
    }
    if (quiescence && blunder) {
      continue;  // ignore own piece captures in quiescence
    }

    // A legal move is a move that's not KO, but when we are in quiescence
    // we only want to count moves that has a capture.
    legal_move_count++;

    if (victim > 0 && !blunder) {
      ext = 1;  // extend captures
    }

    if (is_repeated(&np, &score, ply)) {
      // Break out of loop.
      goto scored;
    }

    { // scote the LMR so that compiler does not complain about next_reduction
      // initialized after goto statement
      // Late move reductions - or LMR
      int next_reduction = 0;
      if (legal_move_count >= LMR_R1 && depth > 2 &&
          victim == 0 && mv != killer_a && mv != killer_b) {
        if (legal_move_count >= LMR_R2) {
          next_reduction = 2;
        } else {
          next_reduction = 1;
        }
      }

      score = -scout_search(&np, -(beta - 1), ext + depth - 1, ply + 1, next_reduction,
          subpv, node_count);
      if (abortf) {
        return 0;
      }
    }

   scored:
    if (score > best_score) {
      best_score = score;
      best_move_index = mv_index;
      pv[0] = mv;
      memcpy(pv + 1, subpv, sizeof(move_t) * (MAX_PLY_IN_SEARCH - 1));
      pv[MAX_PLY_IN_SEARCH - 1] = 0;

      if (score >= beta) {
        if (mv != killer[ply][0]) {
          killer[ply][1] = killer[ply][0];
          killer[ply][0] = mv;
        }
        break;
      }
    }
  }

  if (quiescence == false) {
    if (mv_index < num_of_moves) {
      mv_index++;   // moves tried
    }
    update_best_move_history(p, best_move_index, move_list, mv_index);
  }
  assert(abs(best_score) != -INF);

  if (best_score < beta) {
    tt_hashtable_put(p->key, depth,
        tt_adjust_score_for_hashtable(best_score, ply), UPPER, 0);
  } else {
    tt_hashtable_put(p->key, depth,
        tt_adjust_score_for_hashtable(best_score, ply), LOWER, pv[0]);
  }

  return best_score;
}

// search principal variation
static score_t searchPV(position_t *p, score_t alpha, score_t beta, int depth,
                        int ply, move_t *pv, uint64_t *node_count) {
  pv[0] = 0;

  // check whether we should abort
  tics++;
  if ((tics & ABORT_CHECK_PERIOD) == 0) {
    if (milliseconds() >= timeout) {
      abortf = true;
      return 0;
    }
  }

  // get transposition table record if available
  ttRec_t *rec = tt_hashtable_get(p->key);
  int hash_table_move = 0;
  if (rec) {
    hash_table_move = tt_move_of(rec);
  }

  score_t best_score = -INF;
  score_t sps = eval(p, false) + HMB;  // stand pat (having-the-move) bonus
  bool quiescence = (depth <= 0);      // are we in quiescence?
  score_t orig_alpha = alpha;

  if (quiescence) {
    best_score = sps;
    if (best_score >= beta) {
      return best_score;
    }
    if (best_score > alpha) {
      alpha = best_score;
    }
  }

  position_t np;  // next position
  // hopefully, more than we will need
  sortable_move_t move_list[MAX_NUM_MOVES];
  // number of moves in list
  int num_of_moves = generate_all(p, move_list, false);

  color_t fctm = color_to_move_of(p);
  int pov = 1 - fctm*2;      // point of view = 1 for white, -1 for black
  move_t killer_a = killer[ply][0];
  move_t killer_b = killer[ply][1];

  // sort special moves to the front
  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    move_t mv = get_move(move_list[mv_index]);
    if (mv == hash_table_move) {
      set_sort_key(&move_list[mv_index], SORT_MASK);
      // move_list[mv_index] |= SORT_MASK << SORT_SHIFT;
    } else if (mv == killer_a) {
      set_sort_key(&move_list[mv_index], SORT_MASK - 1);
      // move_list[mv_index] |= (SORT_MASK - 1) << SORT_SHIFT;
    } else if (mv == killer_b) {
      set_sort_key(&move_list[mv_index], SORT_MASK - 2);
      // move_list[mv_index] |= (SORT_MASK - 2) << SORT_SHIFT;
    } else {
      ptype_t  pce = ptype_of(mv);
      rot_t    ro  = rot_of(mv);   // rotation
      square_t fs  = from_square(mv);
      int      ot  = ORI_MASK & (ori_of(p->board[fs]) + ro);
      square_t ts  = to_square(mv);
      set_sort_key(&move_list[mv_index], best_move_history[fctm][pce][ts][ot]);
    }
  }

  move_t subpv[MAX_PLY_IN_SEARCH];
  score_t score;

  bool sortme = true;
  int legal_move_count = 0;
  int mv_index;  // used outside of the loop
  int best_move_index = 0;   // index of best move found

  for (mv_index = 0; mv_index < num_of_moves; mv_index++) {
    subpv[0] = 0;

    // on the fly sorting
    if (sortme) {
      for (int j = mv_index + 1; j < num_of_moves; j++) {
        if (move_list[j] > move_list[mv_index]) {
          sortable_move_t tmp = move_list[j];
          move_list[j] = move_list[mv_index];
          move_list[mv_index] = tmp;
        }
      }
      if (sort_key(move_list[mv_index]) == 0) {
        sortme = false;
      }
    }

    move_t mv = get_move(move_list[mv_index]);
    if (TRACE_MOVES) {
      print_move_info(mv, ply);
    }

    int ext = 0;           // extensions
    bool blunder = false;  // shoot our own piece

    (*node_count)++;
    piece_t victim = make_move(p, &np, mv);  // make the move baby!
    if (victim == KO) {
      continue;
    }

    if (is_game_over(victim, &score, pov, ply)) {
      goto scored;
    }

    if (victim == 0 && quiescence) {
      continue;   // ignore noncapture moves in quiescence
    }
    if (color_of(np.victim) == fctm) {
      blunder = true;
    }
    if (quiescence && blunder) {
      continue;  // ignore own piece captures in quiescence
    }

    // A legal move is a move that's not KO, but when we are in quiescence
    // we only want to count moves that has a capture.
    legal_move_count++;
    if (victim > 0 && !blunder) {
      ext = 1;  // extend captures
    }

    if (is_repeated(&np, &score, ply)) {
      goto scored;
    }

    // first move?
    if (legal_move_count == 1 || quiescence) {
      score = -searchPV(&np, -beta, -alpha, ext + depth - 1, ply + 1,
                        subpv, node_count);
      if (abortf) {
        return 0;
      }
    } else {
      score = -scout_search(&np, -alpha, ext + depth - 1, ply + 1, 0,
                            subpv, node_count);
      if (abortf) {
        return 0;
      }
      if (score > alpha) {
        score = -searchPV(&np, -beta, -alpha, ext + depth - 1, ply + 1,
                          subpv, node_count);
        if (abortf) {
          return 0;
        }
      }
    }

   scored:
    if (score > best_score) {
      best_score = score;
      best_move_index = mv_index;
      pv[0] = mv;
      memcpy(pv+1, subpv, sizeof(move_t) * (MAX_PLY_IN_SEARCH - 1));
      pv[MAX_PLY_IN_SEARCH - 1] = 0;

      if (score > alpha) {
        alpha = score;
      }
      if (score >= beta) {
        if (mv != killer[ply][0]) {
          killer[ply][1] = killer[ply][0];
          killer[ply][0] = mv;
        }
        break;
      }
    }
  }

  if (quiescence == false) {
    if (mv_index < num_of_moves) {
      mv_index++;   // moves tried
    }
    update_best_move_history(p, best_move_index, move_list, mv_index);
  }
  assert(abs(best_score) != -INF);

  if (best_score <= orig_alpha) {
    tt_hashtable_put(p->key, depth,
        tt_adjust_score_for_hashtable(best_score, ply), UPPER, 0);
  } else if (best_score >= beta) {
    tt_hashtable_put(p->key, depth,
        tt_adjust_score_for_hashtable(best_score, ply), LOWER, pv[0]);
  } else {
    tt_hashtable_put(p->key, depth,
        tt_adjust_score_for_hashtable(best_score, ply), EXACT, pv[0]);
  }

  return best_score;
}

score_t searchRoot(position_t *p, score_t alpha, score_t beta, int depth,
                   int ply, move_t *pv, uint64_t *node_count, FILE *OUT) {
  // check whether we should abort
  tics++;
  if ((tics & ABORT_CHECK_PERIOD) == 0) {
    if (milliseconds() >= timeout) {
      abortf = true;
      return 0;
    }
  }

  static int num_of_moves = 0;                     // number of moves in list
  // hopefully, more than we will need
  static sortable_move_t move_list[MAX_NUM_MOVES];

  if (depth == 1) {
    // we are at depth 1; generate all possible moves
    num_of_moves = generate_all(p, move_list, false);
    // shuffle the list of moves. IMP: Commenting out the following loop is a significant performance gain.
    for (int i = 0; i < num_of_moves; i++) {
      int r = myrand() % num_of_moves;
      sortable_move_t tmp = move_list[i];
      move_list[i] = move_list[r];
      move_list[r] = tmp;
    }
  }

  score_t best_score = -INF;
  move_t subpv[MAX_PLY_IN_SEARCH];
  color_t fctm = color_to_move_of(p);
  int pov = 1 - fctm * 2;  // pov = 1 for White, -1 for Black

  position_t np;            // next position
  score_t score;

  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    move_t mv = get_move(move_list[mv_index]);

    if (TRACE_MOVES) {
      print_move_info(mv, ply);
    }

    (*node_count)++;
    piece_t x = make_move(p, &np, mv);  // make the move baby!
    if (x == KO) {
      continue;  // not a legal move
    }

    if (is_game_over(x, &score, pov, ply)) {
      subpv[0] = 0;
      goto scored;
    }

    if (is_repeated(&np, &score, ply)) {
      subpv[0] = 0;
      goto scored;
    }

    // first move?
    if (mv_index == 0 || depth == 1) {
      score = -searchPV(&np, -beta, -alpha, depth - 1, ply + 1,
                        subpv, node_count);
      if (abortf) {
        return 0;
      }
    } else {
      score = -scout_search(&np, -alpha, depth - 1, ply + 1, 0,
                            subpv, node_count);
      if (abortf) {
        return 0;
      }

      if (score > alpha) {
        score = -searchPV(&np, -beta, -alpha, depth - 1, ply + 1,
                          subpv, node_count);
        if (abortf) {
          return 0;
        }
      }
    }

  scored:
    if (score > best_score) {
      best_score = score;
      pv[0] = mv;
      memcpy(pv+1, subpv, sizeof(move_t) * (MAX_PLY_IN_SEARCH - 1));
      pv[MAX_PLY_IN_SEARCH - 1] = 0;

      // ----- do the UCI output thing here -----
      double et = elapsed_time();
      char   pvbuf[MAX_PLY_IN_SEARCH * MAX_CHARS_IN_MOVE];
      getPV(pv, pvbuf);
      if (et < 0.00001) {
        et = 0.00001;  // hack so that we don't divide by 0
      }
      uint64_t nps = *node_count / et;

      fprintf(OUT, "info depth %d move_no %d time (microsec) %d nodes %" PRIu64 
              " nps %" PRIu64 "\n",
              depth, mv_index + 1, (int) (et * 1000), *node_count, nps);
      fprintf(OUT, "info score cp %d pv %s\n", score, pvbuf);

      // -------------------------------------------------------------------
      // slide best move into front of list
      // ----------------------------------
      for (int j = mv_index; j > 0; j--) {
        move_list[j] = move_list[j - 1];
      }
      move_list[0] = mv;
    }

    if (score > alpha) {
      alpha = score;
    }
    if (score >= beta) {
      break;
    }
  }

  return best_score;
}
