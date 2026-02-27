#include "Search.h"
#include <algorithm>
#include <iostream>
#include <cstring>

Search::Search() : shouldStop(false), timeLimit(3.0), tt(TT_SIZE) {
    clearHeuristics();
}

void Search::clearHeuristics() {
    memset(killers, 0, sizeof(killers));
    memset(history, 0, sizeof(history));
}

bool Search::timeUp() const {
    if (shouldStop) return true;
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime).count();
    return elapsed >= timeLimit;
}

void Search::storeTT(uint64_t key, int depth, int score, Move best, int flag) {
    int idx = key % TT_SIZE;
    auto& e = tt[idx];
    if (e.key == key && e.depth > depth) return; // keep deeper
    e.key = key; e.depth = depth; e.score = score; e.best = best; e.flag = flag;
}

TTEntry* Search::probeTT(uint64_t key) {
    int idx = key % TT_SIZE;
    if (tt[idx].key == key) return &tt[idx];
    return nullptr;
}

int Search::scoreCapture(const Board& board, Move m) {
    int cap = board.pieceAt(m.to());
    int atk = board.pieceAt(m.from());
    if (!cap) return 0;
    // MVV-LVA
    return Eval::materialValue(cap)*10 - Eval::materialValue(atk);
}

int Search::moveScore(const Board& board, Move m, Move ttMove, int ply) {
    if (m == ttMove) return 100000;
    int cap = board.pieceAt(m.to());
    if (cap) return 10000 + scoreCapture(board, m);
    if (m.flags()==FLAG_EP) return 9000;
    if (m.flags()==FLAG_PROMO) return 8000 + m.promo()*100;
    if (ply < 128) {
        if (killers[ply][0] == m) return 7000;
        if (killers[ply][1] == m) return 6900;
    }
    return history[m.from()][m.to()];
}

void Search::orderMoves(Board& board, std::vector<Move>& moves, Move ttMove, int ply) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return moveScore(board, a, ttMove, ply) > moveScore(board, b, ttMove, ply);
    });
}

int Search::quiesce(Board& board, int alpha, int beta, int ply) {
    nodesSearched++;
    if (timeUp()) return alpha;

    int stand = Eval::evaluate(board);
    if (board.sideToMove() == BLACK) stand = -stand;

    if (stand >= beta) return beta;
    if (stand > alpha) alpha = stand;

    auto caps = MoveGen::generateCaptures(board);
    // Order captures by MVV-LVA
    std::sort(caps.begin(), caps.end(), [&](Move a, Move b) {
        return scoreCapture(board, a) > scoreCapture(board, b);
    });

    for (auto& m : caps) {
        if (!board.makeMove(m)) continue;
        int score = -quiesce(board, -beta, -alpha, ply+1);
        board.unmakeMove();
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta, int ply, bool nullMoveAllowed) {
    if (timeUp()) return alpha;
    nodesSearched++;

    if (board.isDraw()) return Eval::DRAW;

    uint64_t key = board.zobrist();
    Move ttMove;
    auto* tte = probeTT(key);
    if (tte && tte->depth >= depth) {
        if (tte->flag == 0) return tte->score;
        if (tte->flag == 1) alpha = std::max(alpha, tte->score);
        if (tte->flag == 2) beta = std::min(beta, tte->score);
        if (alpha >= beta) return tte->score;
        ttMove = tte->best;
    } else if (tte) {
        ttMove = tte->best;
    }

    if (depth <= 0) return quiesce(board, alpha, beta, ply);

    bool inCheck = board.isInCheck(board.sideToMove());

    // Null move pruning
    if (nullMoveAllowed && !inCheck && depth >= 3) {
        // Check we have non-pawn material
        Color c = board.sideToMove();
        int minors = board.countPiece(c,KNIGHT)+board.countPiece(c,BISHOP)+
                     board.countPiece(c,ROOK)+board.countPiece(c,QUEEN);
        if (minors > 0) {
            // Make null move by just switching side
            board.stateHistory.push_back(board.getState());
            // Manually toggle side - we need to access Board internals
            // Instead we'll use a different approach - skip if board doesn't support it
            // For now, skip null move and implement via a passthrough
            board.stateHistory.pop_back();
        }
    }

    auto moves = MoveGen::generateMoves(board);
    orderMoves(board, moves, ttMove, ply);

    if (moves.empty()) {
        if (inCheck) return -(Eval::CHECKMATE - ply); // checkmate
        return Eval::DRAW; // stalemate
    }

    int origAlpha = alpha;
    Move bestMove;
    int moveCount = 0;

    for (auto& m : moves) {
        if (!board.makeMove(m)) continue;
        moveCount++;

        int score;
        bool isCapture = (board.stateHistory.back().capturedPiece != 0);
        
        // Late Move Reductions
        int newDepth = depth - 1;
        if (moveCount > 4 && depth >= 3 && !inCheck && !isCapture && m.flags()!=FLAG_PROMO) {
            int R = 1 + (moveCount > 8 ? 1 : 0) + (depth > 6 ? 1 : 0);
            score = -alphaBeta(board, newDepth - R, -alpha-1, -alpha, ply+1, true);
            if (score > alpha) {
                score = -alphaBeta(board, newDepth, -beta, -alpha, ply+1, true);
            }
        } else if (moveCount > 1) {
            // PVS
            score = -alphaBeta(board, newDepth, -alpha-1, -alpha, ply+1, true);
            if (score > alpha && score < beta) {
                score = -alphaBeta(board, newDepth, -beta, -alpha, ply+1, true);
            }
        } else {
            score = -alphaBeta(board, newDepth, -beta, -alpha, ply+1, true);
        }

        board.unmakeMove();

        if (timeUp()) break;

        if (score > alpha) {
            alpha = score;
            bestMove = m;
            if (ply == 0) {
                bestMoveFound = m;
                lastScore = score;
            }
        }
        if (alpha >= beta) {
            // Killer move
            if (!board.pieceAt(m.to()) && ply < 128) {
                killers[ply][1] = killers[ply][0];
                killers[ply][0] = m;
            }
            // History heuristic
            history[m.from()][m.to()] += depth * depth;
            break;
        }
    }

    if (!timeUp() && !bestMove.isNull()) {
        int flag = (alpha <= origAlpha) ? 2 : (alpha >= beta) ? 1 : 0;
        storeTT(key, depth, alpha, bestMove, flag);
    }

    return alpha;
}

Move Search::findBestMove(Board& board, double timeLimitSec, int maxDepth) {
    shouldStop = false;
    startTime = std::chrono::steady_clock::now();
    timeLimit = timeLimitSec;
    nodesSearched = 0;
    depthReached = 0;
    clearHeuristics();

    // Check for single legal move
    auto legalMoves = MoveGen::generateLegalMoves(board);
    if (legalMoves.empty()) return Move();
    if (legalMoves.size() == 1) return legalMoves[0];

    bestMoveFound = legalMoves[0];
    lastScore = 0;

    // Iterative deepening
    for (int depth = 1; depth <= maxDepth; depth++) {
        Move prevBest = bestMoveFound;
        int prevScore = lastScore;

        int score = alphaBeta(board, depth, -Eval::CHECKMATE, Eval::CHECKMATE, 0, true);

        if (timeUp()) {
            bestMoveFound = prevBest;
            lastScore = prevScore;
            break;
        }

        depthReached = depth;

        // Checkmate found
        if (score >= Eval::CHECKMATE - 200) break;
    }

    return bestMoveFound;
}
