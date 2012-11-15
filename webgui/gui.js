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


$(document).ready(function () {

  var board = new Board();
  var computerColor = 'black';
  
  // Draw board label
  var wrapperPosition = $('#board-wrapper').position();
  var left = wrapperPosition.left
  var top = wrapperPosition.top + 20
  for (var i = NUM_ROWS; i > 0; i--) {
    var label = $('<div>' + (i-1) + '</div>');
    label.addClass('board-label');
    label.appendTo('#board-wrapper');
    label.css({ top: top + (NUM_ROWS - i) * PIECE_SIZE, left: left });
  }
  
  left += 40;
  top -= 10;
  for (var i = 0; i < NUM_COLS; i++) {
    var label = $('<div>' + COL_NAMES[i] + '</div>');
    label.addClass('board-label');
    label.appendTo('#board-wrapper');
    label.css({ top: top + NUM_ROWS * PIECE_SIZE, left: left + i * PIECE_SIZE });
  }
  
  // Draw lines
  var b_canvas = document.getElementById('cboard');
  var ctx = b_canvas.getContext('2d');
    
  ctx.fillStyle = '#F0F0F0';  // board color
  ctx.fillRect (0, 0, PIECE_SIZE * NUM_COLS, PIECE_SIZE * NUM_ROWS);

  ctx.strokeStyle = '#A0A0A0';  // line color
  for (var i = 1; i < NUM_COLS; i++) {
    ctx.moveTo(PIECE_SIZE * i, 0);
    ctx.lineTo(PIECE_SIZE * i, PIECE_SIZE * NUM_ROWS);
  }
  for (var i = 1; i <= NUM_ROWS; i++) {
    ctx.moveTo(0, PIECE_SIZE * i);
    ctx.lineTo(PIECE_SIZE * NUM_COLS, PIECE_SIZE * i);
  }
  ctx.stroke();
  
  // Create laser canvas
  var laserCanvas = document.createElement('canvas');
  laserCanvas.height = PIECE_SIZE * NUM_ROWS;
  laserCanvas.width = PIECE_SIZE * NUM_COLS;
  laserCanvas.style.position = 'absolute';
  laserCanvas.style.zIndex = PIECE_SIZE;
  laserCanvas.id = 'laserCanvas';
  
  var parent = $('#board-parent').position();
  $('#board-parent').append(laserCanvas);
  $('#laserCanvas').css({ top: parent.top + 3, left: parent.left + 3});
     
  // Create board
  board.setBoardFenHist('startpos');
  board.drawBoard();
  enableHistoryButtons(true);
  updateUI();
  
  $('#move').keypress(function(event) {
    if (event.which == 13) {
      event.preventDefault();
      move();
    }
  });
  
  function setBoardFenHist() {
    var text = $('#fen-text').val();
    newboard = new Board();
    if (newboard.setBoardFenHist(text)) {
      board = newboard;
      board.drawBoard();
      enableHistoryButtons(true);
      updateUI();
    } else {
      alert('fail');
    }
  };
  
  function stepTo(step) {
    var text = board.getBoardFenHistStep(step);
    newboard = new Board();
    if (newboard.setBoardFenHist(text)) {
      newboard.history = board.history;
      newboard.blacktime = board.blacktime;
      newboard.whitetime = board.whitetime;
      board = newboard;
      board.drawBoard();
      enableHistoryButtons(true);
      updateUI();
    } else {
      alert('fail');
    }
  };
  
  function stepFirst() {
    stepTo(0);
  }
  
  function stepPrev() {
    stepTo(board.historyActiveLen - 1);
  }
  
  function stepNext() {
    stepTo(board.historyActiveLen + 1);
  }
  
  function stepLast() {
    stepTo(board.history.length);
  }
  
  function undo() {
    // undo button usable while game active
    // similar to stepTo(board.historyActiveLen - 2) in some sense
    var text = board.getBoardFenHistStep(board.historyActiveLen - 2);
    newboard = new Board();
    if (newboard.setBoardFenHist(text)) {
      newboard.blacktime = board.blacktime;
      newboard.whitetime = board.whitetime;
      board = newboard;
      board.drawBoard();
      updateUI();
    } else {
      alert('fail');
    }
  }
  
  function move() {
    var text = $('#move').val();
    var moveUnsafeCallback = function() {alert('MOVE UNSAFE!')};
    var moveInvalidCallback = function() {};
    var moveAnimatedCallback = function(lastMove) {
      return function() {
        $('#cancel-button').removeAttr('disabled');
        $('#laser-button').removeAttr('disabled');
        $('#laser-button').addClass('highlight');
        $('#move').attr('disabled', 'disabled');
        $('#move-button').attr('disabled', 'disabled');
        $('#undo-button').attr('disabled', 'disabled');
        $('#clock-stop-button').attr('disabled', 'disabled');
        $('#clock-reset-button').attr('disabled', 'disabled');
        board.lookAhead(lastMove, moveUnsafeCallback);
        board.drawLaser('rgba(221,221,51,0.9)', 'rgba(170,220,160,0.4)');
      };
    }(text);
    board.move(text, moveAnimatedCallback, moveInvalidCallback);
    $('#move').val('');
  };
  
  function cancel() {
    $('#cancel-button').attr('disabled','disabled');
    $('#laser-button').attr('disabled','disabled');
    $('#laser-button').removeClass('highlight');
    $('#move').removeAttr('disabled');
    $('#move-button').removeAttr('disabled');
    $('#undo-button').removeAttr('disabled');
    $('#clock-stop-button').removeAttr('disabled');
    board.revertPotentialMove();
  }
  
  function shootLaser() {
    $('#cancel-button').attr('disabled','disabled');
    $('#laser-button').attr('disabled','disabled');
    $('#laser-button').removeClass('highlight');
    $('#clock-stop-button').removeAttr('disabled');
    $('#clock-reset-button').removeAttr('disabled');
    board.commitPotentialMove();
    board.shootLaser();
  }
  
  function timeToString(time) {
    var negative = (time < 0);
    if (negative) {
      time = -time;
    }
    seconds = time % 60;
    minutes = (time - seconds) / 60;
    if (negative) {
      minutes = '-' + minutes;
    }
    if (seconds < 10) {
      seconds = '0' + seconds;
    }
    return minutes + ':' + seconds;
  }
  
  // stopTimeFlag is a function without argument that returns true
  //              when countdown should be stopped, false otherwise
  // To start the countdown, call updateTime(stopTimeFlag)();
  // Calling updateTime()(); would start an infinite countdown.
  function updateTime(stopTimeFlag) {
    return function() {
      if (!stopTimeFlag || !stopTimeFlag()) {
        if (board.turn == 'black') {
          board.blacktime--;
        } else {
          board.whitetime--;
        }
        $('#whitetime').html(timeToString(board.whitetime));
        $('#blacktime').html(timeToString(board.blacktime));
        setTimeout(updateTime(stopTimeFlag), 1000);
      }
    }
  }
  
  function resetTime() {
    board.blacktime = board.DEFAULT_START_TIME;
    board.whitetime = board.DEFAULT_START_TIME;
    $('#whitetime').html(timeToString(board.whitetime));
    $('#blacktime').html(timeToString(board.blacktime));
  }
  
  function enableHistoryButtons(enabled) {
    if (!enabled || board.historyActiveLen === 0) {
      $('#hist-first-button').attr('disabled','disabled');
      $('#hist-prev-button').attr('disabled','disabled');
    } else {
      $('#hist-first-button').removeAttr('disabled');
      $('#hist-prev-button').removeAttr('disabled');
    }
    if (!enabled || board.historyActiveLen === board.history.length) {
      $('#hist-next-button').attr('disabled','disabled');
      $('#hist-last-button').attr('disabled','disabled');
    } else {
      $('#hist-next-button').removeAttr('disabled');
      $('#hist-last-button').removeAttr('disabled');
    }
    if (enabled) {
      $('#clock-start-button').removeAttr('disabled');
      $('#clock-stop-button').attr('disabled','disabled');
      $('#fen-text').removeAttr('disabled');
      $('#fen-button').removeAttr('disabled');
    } else {
      $('#clock-start-button').attr('disabled','disabled');
      $('#clock-stop-button').removeAttr('disabled');
      $('#fen-text').attr('disabled','disabled');
      $('#fen-button').attr('disabled','disabled');
    }
  }
  
  function updateUI() {
    // HISTORY pane
    $('#move-no').html('');
    $('#black-move').html('');
    $('#white-move').html('');
    for (var i = 0; i < board.history.length; i++) {
      // turn number
      if (i % 2 == 0) {
        var mv = 1 + Math.floor(i/2);
        var moveNumItem = $('<div>');
        moveNumItem.html(mv + '.&nbsp;');
        if (i >= board.historyActiveLen) {
          moveNumItem.addClass('historyInactive');
        }
        $('#move-no').append(moveNumItem);
      }
      // moves
      var historyItem = $('<div>');
      historyItem.text(board.history[i]);
      if (i >= board.historyActiveLen) {
        historyItem.addClass('historyInactive');
      }
      if (i % 2 == 0) {
        $('#black-move').append(historyItem);
      } else {
        $('#white-move').append(historyItem);
      }
    }
    // REP pane  
    $('#fen-rep-hist').text(board.getBoardFenHistCurrent());
    $('#fen-rep-nohist').text(board.getBoardFen());
  }
  
  // Board action when player is switched
  $('#board-parent').bind('switchPlayer', function() {
    if (board.turn === computerColor) {
      $('#move').attr('disabled', 'disabled');
      $('#move-button').attr('disabled', 'disabled');
      $('#undo-button').attr('disabled', 'disabled');
      args = {
        position: board.getStartBoardFen(),
        moves: board.history.join(' '),
        whitetime: board.whitetime * 1000,
        blacktime: board.blacktime * 1000,
        blackinc: board.blackinc * 1000,
        whiteinc: board.whiteinc * 1000,
      };
      
      $.post('/move/', args, function(data) {
        var cb = function(move) {
          return function() {alert('BOT MADE AN INVALID MOVE '+move+'!')};
        };
        board.move(data.move, shootLaser, cb(data.move));
      }, 'json');
    } else {
      $('#move').removeAttr('disabled');
      $('#move-button').removeAttr('disabled');
      $('#undo-button').removeAttr('disabled');
    }
    enableHistoryButtons(false);
    updateUI();
  });
  
  // Set MOVE button actions
  $('#move-button').click(move);
  $('#undo-button').click(undo);
  $('#cancel-button').click(cancel);
  $('#laser-button').click(shootLaser);
  
  // Set CLOCK button actions
  $('#clock-start-button').click( function () {
    board.history = board.history.slice(0,board.historyActiveLen);
    enableHistoryButtons(false);
    updateUI();
    var stopTimeFlagVar = false;
    var stopTimeFlag = function () {
      return stopTimeFlagVar;
    };
    var stopTime = function () {
      stopTimeFlagVar = true;
      enableHistoryButtons(true);
      $('#move').attr('disabled', 'disabled');
      $('#move-button').attr('disabled', 'disabled');
      $('#undo-button').attr('disabled', 'disabled');
    };
    var stopAndResetTime = function () {
      stopTime();
      resetTime();
    }
    updateTime(stopTimeFlag)();
    $('#clock-stop-button').click(stopTime);
    $('#clock-reset-button').click(stopAndResetTime);
    $('#board-parent').trigger('switchPlayer');
  });
  $('#clock-reset-button').click(resetTime);
  
  // Set HISTORY button actions
  $('#hist-first-button').click(stepFirst);
  $('#hist-prev-button').click(stepPrev);
  $('#hist-next-button').click(stepNext);
  $('#hist-last-button').click(stepLast);
  
  // Set REPRESENTATION button actions
  $('#fen-button').click(setBoardFenHist);
  
});

