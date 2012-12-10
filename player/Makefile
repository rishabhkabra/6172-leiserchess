CXX = icpc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c abort.cpp
OBJ := $(addsuffix .o, $(basename $(SRC)))

CXXFLAGS := -c -Wall
TEST_DIR := ../tests/
TEST_CONFIG_FILE := basic_old_new_timed
BETA2_DIR := ../player/beta2/
PLAYER_DIR := ../player/

DEPTH := 6
MOREDEPTH := 8
REALTIME := 250000

ifeq ($(DEBUG),1)
	CXXFLAGS := -DDEBUG -O0 -g $(CXXFLAGS)
else
	CXXFLAGS := -DNDEBUG -O2 -xSSE4.2 -ipo $(CXXFLAGS)
endif

ifeq ($(PROFILE),1)
	CXXFLAGS += -pg
endif

%.o : %.c
	$(CXX) -std=c99 $(CXXFLAGS) $< -o $@

all: clean leiserchess

autotest: runautotest

top5autotest:
	rm -f $(BETA2_DIR)*.pgn
	rm -f $(BETA2_DIR)leiserchess
	cp $(PLAYER_DIR)leiserchess $(BETA2_DIR)leiserchess
	java -jar $(TEST_DIR)lauto.jar $(BETA2_DIR)b2_r1_config &
	java -jar $(TEST_DIR)lauto.jar $(BETA2_DIR)b2_r2_config &
	java -jar $(TEST_DIR)lauto.jar $(BETA2_DIR)b2_r3_config &
	java -jar $(TEST_DIR)lauto.jar $(BETA2_DIR)b2_r4_config &
	java -jar $(TEST_DIR)lauto.jar $(BETA2_DIR)b2_r5_config
	echo "Done"

elo:
	$(TEST_DIR)pgnrate.tcl -anchor newDepth3 -elo 3000 $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

top5elo:
	$(TEST_DIR)pgnrate.tcl $(BETA2_DIR)b2_r1_config.pgn
	$(TEST_DIR)pgnstats $(BETA2_DIR)b2_r1_config.pgn
	$(TEST_DIR)pgnrate.tcl $(BETA2_DIR)b2_r2_config.pgn
	$(TEST_DIR)pgnstats $(BETA2_DIR)b2_r2_config.pgn
	$(TEST_DIR)pgnrate.tcl $(BETA2_DIR)b2_r3_config.pgn
	$(TEST_DIR)pgnstats $(BETA2_DIR)b2_r3_config.pgn
	$(TEST_DIR)pgnrate.tcl $(BETA2_DIR)b2_r4_config.pgn
	$(TEST_DIR)pgnstats $(BETA2_DIR)b2_r4_config.pgn
	$(TEST_DIR)pgnrate.tcl $(BETA2_DIR)b2_r5_config.pgn
	$(TEST_DIR)pgnstats $(BETA2_DIR)b2_r5_config.pgn

runautotest:
	rm -f $(TEST_DIR)$(TEST_CONFIG_FILE).pgn
	java -jar $(TEST_DIR)lauto.jar $(TEST_DIR)$(TEST_CONFIG_FILE)
	echo "Done"
	grep -i "Illegal" $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

time: 	createinput
	perf stat ./leiserchess < input.txt

createinput:
	echo go depth $(DEPTH) > input.txt;
	echo move f0e0 >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo move e9U >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo move j4R >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo move f5g4 >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo move f4e5 >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo move g4R >> input.txt;
	echo go depth $(DEPTH) >> input.txt;
	echo quit >> input.txt;

moretime:
	echo go depth $(MOREDEPTH) > input.txt;
	echo move f0e0 >> input.txt;
	echo go depth $(MOREDEPTH) >> input.txt;
	echo move e9U >> input.txt;
	echo go depth $(MOREDEPTH) >> input.txt;
	echo quit >> input.txt;
	perf stat ./leiserchess < input.txt

realtime:
	echo go time $(REALTIME) > input.txt;
	echo quit >> input.txt
	perf stat ./leiserchess < input.txt

leiserchess: $(OBJ) leiserchess.o
ifeq ($(PROFILE),1)
	$(CXX) $(OBJ) leiserchess.o -o $@ -pg
	./leiserchess < input.txt
	gprof ./leiserchess gmon.out
else
	$(CXX) $(OBJ) leiserchess.o -o $@
endif
clean :
	rm -f *.o *~ $(TARGET)
