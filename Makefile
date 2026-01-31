CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = chess
SRC = chess.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
