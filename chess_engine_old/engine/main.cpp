#include "cli/CLI.h"
#include "board/Board.h"
#include "movegen/MoveGen.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Check for perft test mode
    if (argc >= 3 && std::string(argv[1]) == "perft") {
        int depth = std::stoi(argv[2]);
        Board board;
        if (argc >= 4) board.loadFEN(argv[3]);
        std::cout << "Running perft(" << depth << ")...\n";
        uint64_t nodes = MoveGen::perft(board, depth);
        std::cout << "Nodes: " << nodes << "\n";
        return 0;
    }

    // Check for FEN mode
    if (argc >= 3 && std::string(argv[1]) == "fen") {
        Board board;
        board.loadFEN(argv[2]);
        board.print(true);
        auto legal = MoveGen::generateLegalMoves(board);
        std::cout << "Legal moves: " << legal.size() << "\n";
        return 0;
    }

    CLI cli;
    cli.run();
    return 0;
}
