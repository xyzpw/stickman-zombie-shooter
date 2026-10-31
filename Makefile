CXXFLAGS = -fsanitize=address -Wall -Iinclude
LDFLAGS = -lraylib

TARGET = zombieshooter

SRC = $(shell find . -type f -name '*.cc')
OBJ = $(shell find . -type f -name '*.hpp')

$(TARGET): $(SRC) $(OBJ)
	g++ $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
