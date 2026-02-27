#pragma once
#include "../board/Board.h"
#include "../movegen/MoveGen.h"
#include "../eval/Eval.h"
#include <chrono>
#include <unordered_map>
#include <vector>
#include <atomic>

struct TTEntry {
    uint64_t key = 0;
    int depth = 0;
    int score = 0;
    Move best;
    int flag = 0; // 0=exact, 1=lower, 2=upper
};

class Search {
public:
    Search();

    // Find best move within time limit (seconds)
    Move findBestMove(Board& board, double timeLimitSec, int maxDepth=64);

    // Stats
    int nodesSearched = 0;
    int depthReached = 0;
    int lastScore = 0;
    Move bestMoveFound;

    void stop() { shouldStop = true; }

private:
    std::atomic<bool> shouldStop;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    double timeLimit;

    // Transposition table
    static constexpr int TT_SIZE = (1<<20); // 1M entries
    std::vector<TTEntry> tt;

    // Killer moves [ply][2]
    Move killers[128][2];
    // History heuristic [from][to]
    int history[64][64];

    void clearHeuristics();
    bool timeUp() const;

    int alphaBeta(Board& board, int depth, int alpha, int beta, int ply, bool nullMoveAllowed);
    int quiesce(Board& board, int alpha, int beta, int ply);

    void orderMoves(Board& board, std::vector<Move>& moves, Move ttMove, int ply);
    int moveScore(const Board& board, Move m, Move ttMove, int ply);

    void storeTT(uint64_t key, int depth, int score, Move best, int flag);
    TTEntry* probeTT(uint64_t key);

    static int scoreCapture(const Board& board, Move m);
};
