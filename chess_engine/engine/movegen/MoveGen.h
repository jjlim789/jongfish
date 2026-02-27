#pragma once
#include "../board/Board.h"
#include <vector>

class MoveGen {
public:
    // Generate all pseudo-legal moves
    static std::vector<Move> generateMoves(const Board& board);
    // Generate only captures (for quiescence)
    static std::vector<Move> generateCaptures(const Board& board);
    // Generate all legal moves
    static std::vector<Move> generateLegalMoves(Board& board);
    // Perft for testing
    static uint64_t perft(Board& board, int depth);
};
