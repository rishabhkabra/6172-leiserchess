CXX = icpc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c abort.cpp
OBJ := $(addsuffix .o, $(basename $(SRC)))

CXXFLAGS := -c -Wall
TEST_DIR := ../tests/

TEST_CONFIG_FILE := basic_old_new

DEPTH := 6

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
	$(TEST_DIR)pgnrate.tcl $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

runautotest:
	java -jar $(TEST_DIR)lauto.jar $(TEST_DIR)$(TEST_CONFIG_FILE)
	echo "Done"
	grep -i "Illegal" $(TEST_DIR)$(TEST_CONFIG_FILE).pgn

time: all
	echo go depth $(DEPTH) > input.txt; echo quit >> input.txt; perf stat ./leiserchess < input.txt

leiserchess: $(OBJ) leiserchess.o
ifeq ($(PROFILE),1)
	$(CXX) $(OBJ) leiserchess.o -o $@ -pg
	echo go depth $(DEPTH) > input.txt; echo quit >> input.txt; ./leiserchess < input.txt
	gprof ./leiserchess gmon.out
else
	$(CXX) $(OBJ) leiserchess.o -o $@
endif
clean :
	rm -f *.o *~ $(TARGET)
