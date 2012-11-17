CXX = icc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c
OBJ := $(addsuffix .o, $(basename $(SRC)))

CFLAGS := -c -Wall
CXXFLAGS := -c -Wall

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

leiserchess : $(OBJ) leiserchess.o
ifeq ($(PROFILE),1)
	$(CXX) $(OBJ) leiserchess.o -o $@ -pg
	echo go depth 4 > input.txt; echo quit >> input.txt; ./leiserchess < input.txt
	gprof ./leiserchess gmon.out
else
	$(CXX) $(OBJ) leiserchess.o -o $@
endif
clean :
	rm -f *.o *~ $(TARGET)
