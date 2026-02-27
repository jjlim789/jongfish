#include "../engine/board/Board.h"
#include "../engine/movegen/MoveGen.h"
#include <iostream>
#include <string>

struct PerftCase {
    std::string fen;
    std::string name;
    int depth;
    uint64_t expected;
};

// Standard perft positions
static const PerftCase CASES[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Start", 1, 20},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Start", 2, 400},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Start", 3, 8902},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Pos2", 1, 48},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Pos2", 2, 2039},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Pos3", 1, 14},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Pos3", 2, 191},
};

int main() {
    int pass=0, fail=0;
    for (auto& tc : CASES) {
        Board board;
        board.loadFEN(tc.fen);
        uint64_t got = MoveGen::perft(board, tc.depth);
        bool ok = (got == tc.expected);
        std::cout << (ok?"[PASS]":"[FAIL]") << " " << tc.name << " d" << tc.depth
                  << ": got " << got << " expected " << tc.expected;
        if (!ok) std::cout << " DIFF=" << (long long)(got-tc.expected);
        std::cout << "\n";
        if (ok) pass++; else fail++;
    }
    std::cout << "\n" << pass << "/" << (pass+fail) << " tests passed.\n";
    return fail > 0 ? 1 : 0;
}
