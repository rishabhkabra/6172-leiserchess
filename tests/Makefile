CXX = icc 
TARGET := pgnstats 
SRC := pgnstats.c
OBJ := $(addsuffix .o, $(basename $(SRC)))


%.o : %.c  
	$(CXX) -c -Wall -O3 $< -o $@

$(TARGET) : $(OBJ)
	$(CXX) $(OBJ) -lm -o $@

clean :
	rm -f *.o *~ $(TARGET)
