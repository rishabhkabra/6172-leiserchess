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

#include <stdbool.h>
#include <stdio.h>

#include "move_gen.h"


static void fen_error(char *fen, int c_count, char *msg) {
  fprintf(stderr, "\nError in FEN string:\n");
  fprintf(stderr, "   %s\n  ", fen);  // Indent 3 spaces
  for (int i = 0; i < c_count; ++i) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "^\n");  // graphical pointer to error character in string
  fprintf(stderr, "%s\n", msg);
}

// Parse FEN description of the board itself
// Returns index of where board description ends or 0 if parsing error.
static int parse_fen_board(position_t *p, char *fen) {
  // Invariant: square (f, r) is last square filled.
  // Fill from last rank to first rank, from first file to last file
  fil_t f = -1;
  rnk_t r = BOARD_WIDTH - 1;

  // Current and next characters from input FEN description
  char c, next_c;

  // Invariant: fen[c_count] is next character to be read
  int c_count = 0;

  // Loop also breaks internally if (f, r) == (BOARD_WIDTH-1, 0)
  while ((c = fen[c_count++]) != '\0') {
    int ori;
    ptype_t typ;

    switch (c) {
      // ignore whitespace until the end
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        if ((f == BOARD_WIDTH - 1) && (r == 0)) {  // our job is done
          return c_count;
        }
        break;

        // digits
      case '1':
        if (fen[c_count] == '0') {
          c_count++;
          c += 9;
        }
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        while (c > '0') {
          if (++f >= BOARD_WIDTH) {
            fen_error(fen, c_count, "Too many squares in rank.\n");
            return 0;
          }
          p->board[square_of(f, r)] = EMPTY << PTYPE_SHIFT;
          c--;
        }
        break;

        // pieces
      case 'N':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'N') {  // White King facing North
          ori = NN;
          typ = KING;
        } else if (next_c == 'W') {  // White Pawn facing NW
          ori = NW;
          typ = PAWN;
        } else if (next_c == 'E') {  // White Pawn facing NE
          ori = NE;
          typ = PAWN;
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        p->board[square_of(f, r)] = (typ << PTYPE_SHIFT) |
                                    (WHITE << COLOR_SHIFT) |
                                    (ori << ORI_SHIFT);
        break;

      case 'n':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'n') {  // Black King facing North
          ori = NN;
          typ = KING;
        } else if (next_c == 'w') {  // White Pawn facing NW
          ori = NW;
          typ = PAWN;
        } else if (next_c == 'e') {  // White Pawn facing NE
          ori = NE;
          typ = PAWN;
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        p->board[square_of(f, r)] = (typ << PTYPE_SHIFT) |
                                    (BLACK << COLOR_SHIFT) |
                                    (ori << ORI_SHIFT);
        break;

      case 'S':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'S') {  // White King facing SOUTH
          ori = SS;
          typ = KING;
        } else if (next_c == 'W') {  // White Pawn facing SW
          ori = SW;
          typ = PAWN;
        } else if (next_c == 'E') {  // White Pawn facing SE
          ori = SE;
          typ = PAWN;
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        p->board[square_of(f, r)] = (typ << PTYPE_SHIFT) |
                                    (WHITE << COLOR_SHIFT) |
                                    (ori << ORI_SHIFT);
        break;

      case 's':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 's') {  // Black King facing South
          ori = SS;
          typ = KING;
        } else if (next_c == 'w') {  // White Pawn facing SW
          ori = SW;
          typ = PAWN;
        } else if (next_c == 'e') {  // White Pawn facing SE
          ori = SE;
          typ = PAWN;
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        p->board[square_of(f, r)] = (typ << PTYPE_SHIFT) |
                                    (BLACK << COLOR_SHIFT) |
                                    (SS << ORI_SHIFT);
        p->kloc[BLACK] = square_of(f, r);
        break;

      case 'E':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'E') {  // White King facing East
          p->board[square_of(f, r)] = (KING << PTYPE_SHIFT) |
                                      (WHITE << COLOR_SHIFT) |
                                      (EE << ORI_SHIFT);
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        break;

      case 'W':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'W') {  // White King facing West
          p->board[square_of(f, r)] = (KING << PTYPE_SHIFT) |
                                      (WHITE << COLOR_SHIFT) |
                                      (WW << ORI_SHIFT);
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        break;

      case 'e':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'e') {  // Black King facing East
          p->board[square_of(f, r)] = (KING << PTYPE_SHIFT) |
                                      (BLACK << COLOR_SHIFT) |
                                      (EE << ORI_SHIFT);
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        break;

      case 'w':
        if (++f >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'w') {  // Black King facing West
          p->board[square_of(f, r)] = (KING << PTYPE_SHIFT) |
                                      (BLACK << COLOR_SHIFT) |
                                      (WW << ORI_SHIFT);
        } else {
          fen_error(fen, c_count+1, "Syntax error");
          return 0;
        }
        break;

        // end of rank
      case '/':
        if (f == BOARD_WIDTH - 1) {
          f = -1;
          if (--r < 0) {
            fen_error(fen, c_count, "Too many ranks");
            return 0;
          }
        } else {
          fen_error(fen, c_count, "Too few squares in rank");
          return 0;
        }
        break;

      default:
        fen_error(fen, c_count, "Syntax error");
        return 0;
        break;
    }  // end switch
  }  // end while

  if ((f == BOARD_WIDTH - 1) && (r == 0)) {
    return c_count;
  } else {
    fen_error(fen, c_count, "Too few squares specified");
    return 0;
  }
}

// returns 0 if no error
static int get_sq_from_str(char *fen, int *c_count, int *sq) {
  char c, next_c;

  while ((c = fen[*c_count++]) != '\0') {
    // skip whitespace
    if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
      continue;
    } else {
      break;  // found nonwhite character
    }
  }

  if (c == '\0') {
    *sq = 0;
    return 0;
  }

  // get file and rank
  if ((c - 'a' < 0) || (c - 'a') > BOARD_WIDTH) {
    fen_error(fen, *c_count, "Illegal specification of last move");
    return 1;
  }
  next_c = fen[*c_count++];
  if (next_c == '\0') {
    fen_error(fen, *c_count, "FEN ended before last move fully specified");

    if ((next_c - '0' < 0) || (next_c - '0') > BOARD_WIDTH) {
      fen_error(fen, *c_count, "Illegal specification of last move");
      return 1;
    }

    *sq = square_of(c - 'a', next_c - '0');
    return 0;
  }
  return 0;
}


// Set up position
int fen_to_pos(position_t *p, char *fen) {
  static  position_t dmy1, dmy2;

  // these sentinels simplify checking previous
  // states without stepping past null pointers.
  dmy1.key = 0;
  dmy1.victim = 1;
  dmy1.history = NULL;

  dmy2.key = 0;
  dmy2.victim = 1;
  dmy2.history = &dmy1;


  p->key = 0;          // hash key
  p->victim = 0;       // piece destroyed by shooter
  p->history = &dmy2;  // history


  if (fen[0] == '\0') {  // Empty FEN => use starting position
    fen = "ss3nw5/3nw6/2nw7/1nw8/nw3nwne4/4SWSE3SE/8SE1/7SE2/6SE3/5SE3NN W";
  }

  int c_count = 0;  // Invariant: fen[c_count] is next char to be read

  for (int i = 0; i < ARR_SIZE; ++i) {
    p->board[i] = INVALID << PTYPE_SHIFT;  // squares are invalid until filled
  }

  c_count = parse_fen_board(p, fen);
  if (!c_count) {
    return 1;  // parse error of board
  }

  // King check

  int Kings[2] = {0, 0};
  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      square_t sq = square_of(f, r);
      piece_t x = p->board[sq];
      ptype_t typ = ptype_of(x);
      if (typ == KING) {
        Kings[color_of(x)]++;
        p->kloc[color_of(x)] = sq;
      }
    }
  }

  if (Kings[WHITE] == 0) {
    fen_error(fen, c_count, "No White Kings");
    return 1;
  } else if (Kings[WHITE] > 1) {
    fen_error(fen, c_count, "Too many White Kings");
    return 1;
  } else if (Kings[BLACK] == 0) {
    fen_error(fen, c_count, "No Black Kings");
    return 1;
  } else if (Kings[BLACK] > 1) {
    fen_error(fen, c_count, "Too many Black Kings");
    return 1;
  }

  char c;
  bool done = false;
  // Look for color to move and set ply accordingly
  while (!done && (c = fen[c_count++]) != '\0') {
    switch (c) {
      // ignore whitespace until the end
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        break;

      // White to move
      case 'W':
      case 'w':
        p->ply = 0;
        done = true;
        break;

      // Black to move
      case 'B':
      case 'b':
        p->ply = 1;
        done = true;
        break;

      default:
        fen_error(fen, c_count, "Must specify White (W) or Black (B) to move");
        return 1;
        break;
    }
  }

  // Look for last move, if it exists
  int lm_from_sq, lm_to_sq, lm_rot;
  if (get_sq_from_str(fen, &c_count, &lm_from_sq) != 0) {  // parse error
    return 1;
  }
  if (lm_from_sq == 0) {   // from-square of last move
    p->last_move = 0;  // no last move specified
    p->key = compute_zob_key(p);
    return 0;
  }

  c = fen[c_count];

  switch (c) {
    case 'R':
    case 'r':
      lm_to_sq = lm_from_sq;
      lm_rot = RIGHT;
      break;
    case 'U':
    case 'u':
      lm_to_sq = lm_from_sq;
      lm_rot = UTURN;
      break;
    case 'L':
    case 'l':
      lm_to_sq = lm_from_sq;
      lm_rot = LEFT;
      break;

    default:  // Not a rotation
      lm_rot = NONE;
      if (get_sq_from_str(fen, &c_count, &lm_to_sq) != 0) {
        return 1;
      }
      break;
  }

  p->last_move = (lm_from_sq << FROM_SHIFT) |
                 (lm_to_sq << TO_SHIFT) |
                 (lm_rot << ROT_SHIFT);
  p->key = compute_zob_key(p);

  return 0;  // everything is okay
}
