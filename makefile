CXX = g++
CXXFLAGS = -Iinclude -Wall -std=c++17

TARGET = bitrieve

SRC = main.cpp
OBJ = $(SRC:.cpp=.o)

.PHONY: all clean

all: clean $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) -lext2fs

%.o: %.cpp include/globals.hpp include/extents.hpp include/snapshot.hpp include/recover.hpp include/delete.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) 
