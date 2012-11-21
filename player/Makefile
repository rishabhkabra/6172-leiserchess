CXX = icpc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c abort.cpp
OBJ := $(addsuffix .o, $(basename $(SRC)))

CXXFLAGS := -c -Wall
TEST_DIR := ../tests/

TEST_CONFIG_FILE := basic_old_new

DEPTH := 7

ifeq ($(DEBUG),1)
	CXXFLAGS := -DDEBUG -O0 -g $(CXXFLAGS)
else
	CXXFLAGS := -DNDEBUG -O3 $(CXXFLAGS)
endif

ifeq ($(PROFILE),1)
	CXXFLAGS += -pg
endif

%.o : %.c
	$(CXX) -std=c99 $(CXXFLAGS) $< -o $@

all: clean leiserchess

autotest: leiserchess runautotest

elo:
	$(TEST_DIR)pgnrate.tcl -anchor newDepth3 -elo 3000 $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

runautotest:
	rm $(TEST_DIR)$(TEST_CONFIG_FILE).pgn
	java -jar $(TEST_DIR)lauto.jar $(TEST_DIR)$(TEST_CONFIG_FILE)
	echo "Done"
	grep -i "Illegal" $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

time: createinput all
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
