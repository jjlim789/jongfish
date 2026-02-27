#pragma once
#include "../board/Board.h"

class Eval {
public:
    // Returns eval in centipawns from White's perspective
    static int evaluate(const Board& board);
    
    // Material values
    static constexpr int PAWN_VAL   = 100;
    static constexpr int KNIGHT_VAL = 320;
    static constexpr int BISHOP_VAL = 330;
    static constexpr int ROOK_VAL   = 500;
    static constexpr int QUEEN_VAL  = 900;
    static constexpr int KING_VAL   = 20000;

    static constexpr int CHECKMATE  = 100000;
    static constexpr int DRAW       = 0;
    
    static int materialValue(int pc);
    static int gamePhase(const Board& board); // 0=endgame, 24=opening
};
