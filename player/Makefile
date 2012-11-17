CXX = icpc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c abort.cpp
OBJ := $(addsuffix .o, $(basename $(SRC)))

CFLAGS := -c -Wall
CXXFLAGS := -c -Wall
TEST_DIR := ../tests/

TEST_CONFIG_FILE := basic_old_new

ifeq ($(DEBUG),1)
	CFLAGS := -DDEBUG -O0 $(CFLAGS)
	CXXFLAGS := -DDEBUG -O0 $(CXXFLAGS)
else
	CFLAGS := -DNDEBUG -O3 $(CFLAGS)
	CXXFLAGS := -DNDEBUG -O3 $(CXXFLAGS)
endif

ifeq ($(PROFILE),1)
	CFLAGS += -pg
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

leiserchess: $(OBJ) leiserchess.o
ifeq ($(PROFILE),1)
	$(CXX) $(OBJ) leiserchess.o -o $@ -pg
	echo go depth 6 > input.txt; echo quit >> input.txt; ./leiserchess < input.txt
	gprof ./leiserchess gmon.out
else
	$(CXX) $(OBJ) leiserchess.o -o $@
endif
clean :
	rm -f *.o *~ $(TARGET)
