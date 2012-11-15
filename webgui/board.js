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

// Board configuration
var STATICS_HOST = '';
var NUM_COLS = 10;
var NUM_ROWS = 10;
var SPECIAL_BOARDS = {'startpos':'ss3nw5/3nw6/2nw7/1nw8/nw3nwne4/4SWSE3SE/8SE1/7SE2/6SE3/5SE3NN W',
                      'endgame':'ss9/10/10/10/10/10/10/10/10/9NN W'};

// GUI configuration
var PIECE_SIZE = 50;
var ANIMATE_DURATION = 1000;
var BOARD_WIDTH = NUM_COLS * PIECE_SIZE;
var BOARD_HEIGHT = NUM_ROWS * PIECE_SIZE;

// Constants
var COL_NAMES = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'];
var DIRECTIONS = [[-1, -1], [-1, 0], [-1, 1], [0, -1], [0, 1], [1, -1], [1, 0], [1, 1]];
var KING_MAP = {'nn':'u', 'ee':'r', 'ss':'d', 'ww':'l'};
var PAWN_MAP = {'nw':'u', 'ne':'r', 'se':'d', 'sw':'l'};
var PLAYER_MAP = {'B':'black', 'W':'white'};
var KING_INV_MAP = {'u':'nn', 'r':'ee', 'd':'ss', 'l':'ww'};
var PAWN_INV_MAP = {'u':'nw', 'r':'ne', 'd':'se', 'l':'sw'};
var PLAYER_INV_MAP = {'black':'B', 'white':'W'};
var KING_LASER_MAP = {
    'u': {'u':'x', 'r':'x', 'd':'x', 'l':'x'},
    'r': {'u':'x', 'r':'x', 'd':'x', 'l':'x'},
    'd': {'u':'x', 'r':'x', 'd':'x', 'l':'x'},
    'l': {'u':'x', 'r':'x', 'd':'x', 'l':'x'}};
var PAWN_LASER_MAP = {
    'u': {'u':'x', 'r':'u', 'd':'l', 'l':'x'},
    'r': {'u':'x', 'r':'x', 'd':'r', 'l':'u'},
    'd': {'u':'r', 'r':'x', 'd':'x', 'l':'d'},
    'l': {'u':'l', 'r':'d', 'd':'x', 'l':'x'}};
    
// Piece ID
var piece_count = 0;

var Piece = function(orientation, color, position) {
  this.id = 'piece-' + piece_count++;
  this.name = '';
  this.orientation = orientation; 
  this.color = color;
  this.position = position; // Ex: a8
  this.pieceImg = null;
  
  this.fenText = function() {
    var fen = (this.name === 'king') ? KING_INV_MAP[this.orientation] : PAWN_INV_MAP[this.orientation];
    return (this.color === 'white') ? fen.toUpperCase() : fen;
  };
  
  this.imageSrc = function() {
    return STATICS_HOST + 'images/' + this.name + '-' + this.color + '.png';
  };
  
  // Position relative to board
  var parentPosition = $('#board-parent').position();
  
  this.left = function() {
    return 2 + parentPosition.left + COL_NAMES.indexOf(this.position[0]) * PIECE_SIZE;
  }
  
  this.top = function() {
    return 2 + parentPosition.top + (NUM_ROWS - this.position[1] - 1) * PIECE_SIZE;
  }
  
  this.draw = function() {
    if (this.pieceImg === null) {
      this.pieceImg = $('<img />');
      this.pieceImg[0].src = this.imageSrc();
      this.pieceImg[0].id = this.id;
      this.pieceImg[0].width = PIECE_SIZE;
      this.pieceImg[0].height = PIECE_SIZE;
      this.pieceImg.addClass('piece');
      
      this.pieceImg.appendTo('#board-parent');
    }
    
    this.pieceImg.css({ top: this.top(), left: this.left() });
        
    var rotate = {'u': 0, 'd': 180, 'l': 270, 'r': 90};
    this.pieceImg.css({rotate: rotate[this.orientation] + 'deg'});
  };

  this.rotate = function(direction, inanimate) {
    // inanimate optional argument, default false
    var orientations = ['u', 'r', 'd', 'l'];
    var idx = orientations.indexOf(this.orientation);
    var rotationString;
    var idxDelta;
    if ((direction === 'R') || (direction === 'r')) {
      idxDelta = 1;
      rotationString = '+=90deg';
    } else if ((direction === 'L') || (direction === 'l')){
      idxDelta = 3;
      rotationString = '-=90deg';
    } else if ((direction === 'U') || (direction === 'u')){
      idxDelta = 2;
      rotationString = '+=180deg';
    } else {
      return;
    }
    idx = (idx + idxDelta) % 4;
    if (!inanimate) {
      this.pieceImg.transition({rotate: rotationString}, ANIMATE_DURATION);
    }
    this.orientation = orientations[idx];
  }
  
  this.move = function(newPosition, inanimate) {
    // inanimate optional argument, default false
    this.position = newPosition;
    if (!inanimate) {
      this.pieceImg.animate({ top: this.top(), left: this.left() }, ANIMATE_DURATION);
    }
  }

  // Laser
  // laserDirection: direction of input laser
  // return: u,d,l,r (output laser), x (dead), s (stop)
  this.laserResult = function(laserDirection) {
    return this.laserMap[this.orientation][laserDirection]
  }
  
  this.remove = function() {
    this.pieceImg.remove();
  }
}

var King = function(orientation, color, position) {
  var that = new Piece(orientation, color, position);
  that.name = 'king';
  that.laserMap = KING_LASER_MAP;

  return that;
};

var Pawn = function(orientation, color, position) {
  var that = new Piece(orientation, color, position);
  that.name = 'pawn';
  that.laserMap = PAWN_LASER_MAP;
  return that;
};

var Board = function() {
  this.board = {};
  for (var i = 0; i < NUM_COLS; i++) {
    this.board[COL_NAMES[i]] = new Array(NUM_ROWS);
  }
  
  this.startBoardFen = '';
  this.history = [];
  this.historyActiveLen = 0;
  this.enforceKoThisTurn = false;
  this.turn = 'white';
  this.potentialMove = '';
  this.potentialRevertMove = '';
  
  // Timing
  this.DEFAULT_START_TIME = 120; // 2 min
  this.DEFAULT_INC_TIME = 10;    // 10 sec
  this.blacktime = this.DEFAULT_START_TIME;
  this.whitetime = this.DEFAULT_START_TIME;
  this.blackinc = this.DEFAULT_INC_TIME;
  this.whiteinc = this.DEFAULT_INC_TIME;
  
  this.getPiece = function (position) {
    var piece = this.board[position[0]][position[1]];
    if (piece) {
      return piece;
    } else {
      return null;
    }
  }
  
  // position = [col #, row #]
  this.getPieceCR = function (position) {
    var piece = this.board[COL_NAMES[position[0]]][position[1]];
    if (piece) {
      return piece;
    } else {
      return null;
    }
  }
  
  // ########## BOARD REP RELATED ##########
  
  // returns starting board status in FEN notation
  // might use special board names instead
  this.getStartBoardFen = function() {
    if (this.startBoardFen in SPECIAL_BOARDS) {
      return this.startBoardFen;
    } else {
      return 'fen ' + this.startBoardFen;
    }
  };
  
  // returns current board status in FEN notation
  this.getBoardFen = function() {
    var rep = '';
    for (var r = NUM_ROWS-1; r >= 0; r--) {
      var blankCount = 0;
      for (var c = 0; c < NUM_COLS; c++) {
        var piece = this.getPiece(COL_NAMES[c] + r);
        if (piece) {
          if (blankCount !== 0) {
            rep += blankCount;
          }
          rep += piece.fenText();
          blankCount = 0;
        } else {
          blankCount++;
        }
      }
      if (blankCount !== 0) {
        rep += blankCount;
      }
      if (r > 0) {
        rep += '/';
      }
    }
    rep += ' ' + PLAYER_INV_MAP[this.turn];
    return rep;
  };
  
  // returns starting board status in FEN notation
  // plus move history, only first [step] turns
  this.getBoardFenHistStep = function(step) {
    var rep = this.startBoardFen;
    if (step > this.history.length) {
      step = this.history.length;
    }
    if (step > 0) {
      rep += '#' + this.history.slice(0,step).join('#');
    }
    return rep;
  }
  
  // returns starting board status in FEN notation
  // plus move history, only first [historyActiveLen] turns
  this.getBoardFenHistCurrent = function() {
    return this.getBoardFenHistStep(this.historyActiveLen);
  }
  
  // returns starting board status in FEN notation
  // plus move history, all turns
  this.getBoardFenHistFull = function() {
    return this.getBoardFenHistStep(this.history.length);
  }
  
  // takes FEN notation describing ONE row e.g. '10', '4NESE1ss2'
  // returns input separated into tokens of strings and integers
  //     e.g. [4, 'NE', 'SE', 1, 'SS', 2]
  // returns null if input is an invalid string
  this.splitFenRow = function(text) {
    var isDigit = function(char) {
      return '0' <= char && char <= '9';
    };
    var isUppercase = function(char) {
      return 'A' <= char && char <= 'Z';
    };
    
    var answer = [];
    var colused = 0;
    var i = 0;
    while (i < text.length) {
      if (isDigit(text.charAt(i))) {
        var j = i+1;
        while (j < text.length && isDigit(text.charAt(j))) {
          j++;
        }
        var value = parseInt(text.slice(i,j));
        answer.push(value);
        colused += value;
        i = j;
      } else {
        if (i+1 < text.length && !isDigit(text.charAt(i+1))
            && isUppercase(text.charAt(i)) === isUppercase(text.charAt(i+1))) {
          answer.push(text.slice(i,i+2));
          colused++;
          i += 2;
        } else {
          return null;
        }
      }
    }
    return (colused == NUM_COLS) ? answer : null;
  };
  
  // takes board status in FEN notation (without move history)
  // sets the board accordingly
  // returns true if board valid, false otherwise
  this.setBoardFen = function(text) {
    for (var i = 0; i < NUM_COLS; i++) {
      this.board[COL_NAMES[i]] = new Array(NUM_ROWS);
    }
    var isUppercase = function(char) {
      return 'A' <= char && char <= 'Z';
    };
    
    text = text.replace(/\s+/g, ''); // Strip spaces
    if (text in SPECIAL_BOARDS) {
      text = SPECIAL_BOARDS[text].replace(/\s+/g, '');
    }
    var turntext = text.slice(text.length-1).toUpperCase();
    if (!(turntext in PLAYER_MAP)) {
        return false;
    }
    var rowtext = text.slice(0,text.length-1).split('/');
    if (rowtext.length === NUM_ROWS) {
      for (var r = NUM_ROWS-1; r >= 0; r--) {
        var cols = this.splitFenRow(rowtext[NUM_ROWS-r-1]);
        if (cols != null) {
          var c = 0;
          var board = this.board;
          cols.forEach(function(e){
            if (typeof e === 'number') {
              c += e;
            } else {
              var color = isUppercase(e.charAt(0)) ? 'white' : 'black';
              var position = COL_NAMES[c] + r;
              var piece = null;
              if (e.charAt(0) == e.charAt(1)) {
                piece = new King(KING_MAP[e.toLowerCase()], color, position);
              } else {
                piece = new Pawn(PAWN_MAP[e.toLowerCase()], color, position);
              }
              board[COL_NAMES[c]][r] = piece;
              c++;
            }
          });
        } else {
          return false;
        }
      }
    } else {
      return false;
    }
    this.turn = PLAYER_MAP[turntext];
    this.clearLaser();
    return true;
  };
  
  // takes board status in FEN notation (with move history)
  // sets the board accordingly
  // returns true if board valid, false otherwise
  this.setBoardFenHist = function(text) {
    text = text.replace(/\s+/g, ''); // Strip spaces
    var textarray = text.split('#');
    if (textarray.length > 0) {
      // Set initial board
      if (!this.setBoardFen(textarray[0])) {
        return false;
      }
      // Set startBoardFen variable
      if (textarray[0] in SPECIAL_BOARDS) {
        this.startBoardFen = textarray[0];
      } else {
        this.startBoardFen = textarray[0].slice(0,textarray[0].length-1) + ' ' + textarray[0].slice(textarray[0].length-1);
      }
      // Play moves in history
      for (var i = 1; i < textarray.length; i++) {
        var koMove = this.enforceKoThisTurn ? this.history[this.history.length-1] : null;
        if (this.isValidMove(textarray[i], this.turn, koMove)) {
          this.history.push(textarray[i]);
          this.move(textarray[i], null, null, true);
          var victim = this.laserTarget(this.turn);
          if (victim) {
            var hitCol = victim.position[0];
            var hitRow = 1*victim.position[1];
            this.board[hitCol][hitRow] = null;
          }
          this.turn = ((this.turn === 'black') ? 'white' : 'black');
        } else {
          return false;
        }
      }
      this.historyActiveLen = this.history.length;
      return true;
    } else {
      return false;
    }
  }
  
  // ########## MOVES RELATED ##########
  
  // switches player and notifies the player to move
  this.switchPlayer = function() {
    if (this.turn === 'black') {
      this.blacktime += this.blackinc;
      this.turn = 'white';
    } else {
      this.turn = 'black';
      this.whitetime += this.whiteinc;
    }
    $('#board-parent').trigger('switchPlayer');
  }
  
  // returns true if position valid: 'a3', 'j9', etc.
  this.isValidPosition = function(text) {
    // check column name
    var okay = false;
    for (var i = 0; i < NUM_COLS; i++) {
      if (COL_NAMES[i] === text[0]) {
        okay = true;
        break;
      }
    }
    if (!okay) {
      return false;
    }
    // check row name
    okay = false;
    for (var i = 0; i < NUM_ROWS; i++) {
      if (i === (1*text[1])) {
        okay = true;
        break;
      }
    }
    return okay;
  }
  
  // returns true if move valid
  // param: text is the move: 'a3R', 'j0j1', etc.
  // param: turn is the color specifying current player's turn: 'white' or 'black'
  // param: ko is the move considered ko, null if no such move (last move destroyed a piece, etc.): 'd4e5', etc.
  this.isValidMove = function(text, turn, ko) {
    if (text.length === 1) {
      return text === '-';
    } else if (text.length === 3 || text.length === 4) {
      // Starting position is valid
      if (!this.isValidPosition(text.slice(0,2))) {
        return false;
      }
      // Starting position contains the player's piece
      var piece = this.getPiece(text.slice(0,2));
      if (!piece || piece.color !== turn) {
        return false;
      }
      if (text.length === 3) {
        // Move is valid
        return ['L','R','U','l','r','u'].indexOf(text[2]) !== -1;
      } else {
        // Destination position is valid
        if (!this.isValidPosition(text.slice(2,4))) {
          return false;
        }
        // Not breaking Ko rule
        if (text === ko) {
          return false;
        }
        // Move is valid
        var col = COL_NAMES.indexOf(text[2])-COL_NAMES.indexOf(text[0]);
        var row = (1*text[3])-(1*text[1]);
        if (col === 0 && row === 0) {
          return true;
        }
        for (var i=0; i<DIRECTIONS.length; i++) {
          if (DIRECTIONS[i][0] === col && DIRECTIONS[i][1] === row) {
            return true;
          }
        }
      }
    }
    return false;
  };

  this.move = function(text, successCallback, failureCallback, inanimate) {
    // inanimate optional argument, default false
    if (text.length === 3) {
      text = text.slice(0,2)+text[2].toUpperCase();
    }
    var koMove = this.enforceKoThisTurn ? this.history[this.history.length-1] : null;
    if (this.isValidMove(text, this.turn, koMove)) {
      if (!inanimate) {
        this.clearLaser();
      }
      this.potentialMove = text;
      var piece = this.getPiece(text.slice(0,2));
      if (text.length === 3) {
        var revertMap = {'L':'R', 'R':'L', 'U':'U'};
        this.potentialRevertMove = text.slice(0,2)+revertMap[text[2]];
        piece.rotate(text[2], inanimate);
      } else if (text.length === 4) {
        var oldC = text[0];
        var oldR = text[1];
        var newC = text[2];
        var newR = text[3];
    
        // Swap if necessary. 
        var destPiece = this.board[newC][newR];
        this.board[newC][newR] = piece;
        this.board[oldC][oldR] = destPiece;

        this.potentialRevertMove = text.slice(2,4)+text.slice(0,2);
        piece.move(text.slice(2,4), inanimate);
        if (destPiece) {
          destPiece.move(text.slice(0,2), inanimate);
        }
      } else {
        this.potentialRevertMove = '-';
      }
      if (successCallback) {
        setTimeout(function(){successCallback();}, ANIMATE_DURATION);
      }
    } else if (failureCallback) {
      failureCallback();
    }
  };
  
  this.clearPotentialMove = function() {
    this.potentialMove = '';
    this.potentialRevertMove = '';
  };
  
  this.revertPotentialMove = function() {
    this.move(this.potentialRevertMove, null, null);
    this.clearPotentialMove();
  };
  
  this.commitPotentialMove = function() {
    this.history.push(this.potentialMove);
    this.historyActiveLen++;
    this.enforceKoThisTurn = (this.laserTarget(this.turn) === null);
    this.clearPotentialMove();
  };
  
  this.findKing = function(color) {
    for (var r = NUM_ROWS-1; r >= 0; r--) {
      for (var c = 0; c < NUM_COLS; c++) {
        var piece = this.getPiece(COL_NAMES[c] + r);
        if (piece && (piece.color === color) && (piece.name === 'king')) {
          return [c, r];
        }
      }
    }
    return null;
  };
  
  this.lookAhead = function(lastMove, unsafeCallback) {
    var myColor = this.turn;
    var theirColor = (myColor === 'white') ? 'black' : 'white';
    var koEnforced = false;
    
    // my own laser
    var casualty = this.laserTarget(myColor);
    if (casualty) {
      koEnforced = true;
      if ((casualty.color === myColor) && (casualty.name === 'king')) {
        unsafeCallback();
        return;
      }
    }
    // opponent's laser after they move
    var newBoard = new Board();
    newBoard.setBoardFen(this.getBoardFen());
    if (casualty) {
      newBoard.board[casualty.position[0]][casualty.position[1]] = null;
    }
    for (var r = NUM_ROWS-1; r>=0; r--) {
      for (var c = 0; c < NUM_COLS; c++) {
        var piece = newBoard.getPieceCR([c,r]);
        if (piece && (piece.color === theirColor)) {
          // move
          for (var i = 0; i < DIRECTIONS.length; i++) {
            var newC = c+DIRECTIONS[i][0];
            var newR = r+DIRECTIONS[i][1];
            if (newC < 0 || newC >= NUM_COLS || newR < 0 || newR >= NUM_ROWS) {
              continue; // out of bound
            }
            var moveString = COL_NAMES[c]+r+COL_NAMES[newC]+newR;
            if (!koEnforced || moveString !== lastMove) { // checks Ko
              var temp = newBoard.board[COL_NAMES[c]][r];
              newBoard.board[COL_NAMES[c]][r] = newBoard.board[COL_NAMES[newC]][newR];
              newBoard.board[COL_NAMES[newC]][newR] = temp;
              var casualty = newBoard.laserTarget(theirColor);
              if (casualty && (casualty.color === myColor) && (casualty.name === 'king')) {
                unsafeCallback();
                return;
              }
              var temp = newBoard.board[COL_NAMES[c]][r];
              newBoard.board[COL_NAMES[c]][r] = newBoard.board[COL_NAMES[newC]][newR];
              newBoard.board[COL_NAMES[newC]][newR] = temp;
            }
          }
          // rotate
          var orientations = ['u', 'r', 'd', 'l'];
          var idx = orientations.indexOf(piece.orientation);
          for (var i = 3; i >= 0; i--) {
            piece.orientation = orientations[(idx+i)%4];
            var newCasualty = newBoard.laserTarget(theirColor);
            if (newCasualty && (newCasualty.color === myColor) && (newCasualty.name === 'king')) {
              unsafeCallback();
              return;
            }
          }
        }
      }
    }
  };

  // ########## LASER RELATED ##########
  
  // returns the piece destroyed if the laser is shot
  // null if no piece destroyed
  // does not actually shoot laser -- board representation does not change
  this.laserTarget = function(color) {
    var position = this.findKing(color);
    if (!position) {
      return null;
    }
    var direction = this.getPieceCR(position).orientation;
    var transform = {
      'u': [0, 1],
      'r': [1, 0],
      'd': [0, -1],
      'l': [-1, 0]
    }
    
    var piece = null;
    while (true) {
      position[0] += transform[direction][0];
      position[1] += transform[direction][1];
      if (position[0] < 0 || position[0] >= NUM_COLS
        || position[1] < 0 || position[1] >= NUM_ROWS) {
        // out of bounds
        return null;
      }
      piece = this.getPieceCR(position);
      if (piece !== null) {
        direction = piece.laserResult(direction);
        
        if (direction === 's') {
          // Stop
          return null;
        } else if (direction === 'x') {
          // Dead
          return piece;
        }
      }
    }
  };
  
  this.shootLaser = function() {
    this.drawLaser('#FF0000', 'rgba(170,220,160,0.9)');
    this.victim = this.laserTarget(this.turn);
    if (this.victim) {
      var hitCol = this.victim.position[0];
      var hitRow = 1*this.victim.position[1];
      this.board[hitCol][hitRow] = null;
    }
    this.switchPlayer();
  };
  
  // ########## DRAWING RELATED ##########
  
  this.drawBoard = function() {
    $('#board-parent img').remove(); // Clear board
    for (var r = NUM_ROWS-1; r >= 0; r--) {
      for (var c = 0; c < NUM_COLS; c++) {
        var piece = this.getPiece(COL_NAMES[c] + r);
        if (piece) {
          piece.draw();
        }
      }
    }
  };
  
  this.posToCoord = function(position) {
    return [PIECE_SIZE/2 + position[0] * PIECE_SIZE, PIECE_SIZE/2 + (NUM_ROWS - position[1] - 1) * PIECE_SIZE];
  };
  
  this.clearLaser = function() {
    if (this.victim) {
      this.victim.remove();
    }
    var laserCanvas = document.getElementById('laserCanvas');
    var ctx = laserCanvas.getContext('2d');
    ctx.clearRect(0, 0, BOARD_WIDTH, BOARD_HEIGHT);
  }
  
  this.drawLaser = function(strokeColor, fillColor) {
    var laserCanvas = document.getElementById('laserCanvas');
    var ctx = laserCanvas.getContext('2d');
    ctx.strokeStyle = strokeColor;
    ctx.lineWidth = 5;
    ctx.clearRect(0, 0, BOARD_WIDTH, BOARD_HEIGHT);
    
    var position = this.findKing(this.turn);

    var coord = this.posToCoord(position);
    ctx.beginPath();
    ctx.moveTo(coord[0], coord[1]);
    
    var direction = this.getPieceCR(position).orientation;
    var transform = {
      'u': [0, 1],
      'r': [1, 0],
      'd': [0, -1],
      'l': [-1, 0]
    }
    
    var piece = null;
    while (true) {
      position[0] += transform[direction][0];
      position[1] += transform[direction][1];
      if (position[0] < 0 || position[0] >= NUM_COLS
        || position[1] < 0 || position[1] >= NUM_ROWS) {
        // out of bound
        coord = this.posToCoord(position);
        ctx.lineTo(coord[0], coord[1]);
        break;
      }
      piece = this.getPieceCR(position);
      if (piece !== null) {
        coord = this.posToCoord(position);
        ctx.lineTo(coord[0], coord[1]);
        
        direction = piece.laserResult(direction);
        
        if (direction === 's') {
          // Stop
          break;
        } else if (direction === 'x') {
          // Dead
          ctx.fillStyle = fillColor;
          ctx.fillRect(position[0] * PIECE_SIZE, (NUM_ROWS - position[1] - 1) * PIECE_SIZE, 48, 48);
          break;
        }
      }
    }
    ctx.stroke();
  };
  
};

