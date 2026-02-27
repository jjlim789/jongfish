CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra
TARGET = chess_engine
TEST_TARGET = perft_test

SRCS = engine/main.cpp \
       engine/board/Board.cpp \
       engine/movegen/MoveGen.cpp \
       engine/eval/Eval.cpp \
       engine/search/Search.cpp \
       engine/cli/CLI.cpp \
       engine/util/PGN.cpp

TEST_SRCS = tests/perft_test.cpp \
            engine/board/Board.cpp \
            engine/movegen/MoveGen.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -Iengine -o $@ $^

$(TEST_TARGET): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -Iengine -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
