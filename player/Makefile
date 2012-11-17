CXX = icpc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c search.c eval.c abort.cpp
OBJ := $(addsuffix .o, $(basename $(SRC)))

%.o : %.c
	$(CXX) -std=c99 -c -DNDEBUG -O3 -Wall $< -o $@

leiserchess : $(OBJ) leiserchess.o
	$(CXX) $(OBJ) leiserchess.o -o $@


clean :
	rm -f *.o *~ $(TARGET)
