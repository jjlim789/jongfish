#pragma once
#include "../board/Board.h"
#include <string>
#include <vector>

class PGN {
public:
    static std::string moveToSAN(Board& board, Move m);
    static Move sanToMove(Board& board, const std::string& san);
    static std::string exportPGN(const std::vector<std::string>& sanMoves,
                                  const std::string& result = "*",
                                  const std::string& white = "White",
                                  const std::string& black = "Black",
                                  const std::string& event = "Chess Engine Game");
    static bool savePGN(const std::string& pgn, const std::string& filename);
};
