#include "CLI.h"
#include "../movegen/MoveGen.h"
#include "../eval/Eval.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <limits>

// Format a score (White-positive centipawns) for display.
// Mate scores: CHECKMATE - ply stored as CHECKMATE-1, CHECKMATE-2, etc.
// We display them as "M1", "M2" (positive = White mates, negative = Black mates).
static std::string formatScore(int score) {
    const int CM = Eval::CHECKMATE;
    if (score > CM - 300) {
        int movesToMate = (CM - score + 1) / 2;
        return "M" + std::to_string(movesToMate);
    }
    if (score < -(CM - 300)) {
        int movesToMate = (CM + score + 1) / 2;
        return "-M" + std::to_string(movesToMate);
    }
    return (score >= 0 ? "+" : "") + std::to_string(score);
}

CLI::CLI() : mode(HUMAN_VS_AI), humanColor(WHITE), aiTime(3.0) {}

void CLI::run() {
    std::cout << "\n+--------------------------------------+\n";
    std::cout << "|      C++ Chess Engine  v1.0          |\n";
    std::cout << "|   Vibe-coded with AI assistance      |\n";
    std::cout << "+--------------------------------------+\n\n";
    selectMode();
    gameLoop();
}

void CLI::selectMode() {
    std::cout << "Select mode:\n";
    std::cout << "  1. Human vs AI\n";
    std::cout << "  2. Bot vs Bot\n";
    std::cout << "Choice: ";
    int choice; std::cin >> choice;
    mode = (choice==2) ? AI_VS_AI : HUMAN_VS_AI;

    if (mode == HUMAN_VS_AI) {
        std::cout << "Play as (w)hite or (b)lack? ";
        char c; std::cin >> c;
        humanColor = (c=='b'||c=='B') ? BLACK : WHITE;
    }

    std::cout << "AI thinking time per move (seconds, default 3): ";
    std::string t; std::cin >> t;
    try { aiTime = std::stod(t); } catch(...) { aiTime = 3.0; }
    if (aiTime<=0) aiTime=3.0;

    std::cout << "\nCommands: 'undo', 'eval', 'flip', 'savepgn <file>', 'perft <depth>', 'quit'\n\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Default: show board from human's perspective
    if (mode == HUMAN_VS_AI && humanColor == BLACK) boardFlipped = true;
}

bool CLI::gameOver(std::string& result) {
    auto legal = MoveGen::generateLegalMoves(board);
    if (legal.empty()) {
        if (board.isInCheck(board.sideToMove())) {
            result = (board.sideToMove()==WHITE) ? "0-1" : "1-0";
            std::cout << "\nCheckmate! " << result << "\n";
        } else {
            result = "1/2-1/2";
            std::cout << "\nStalemate! Draw.\n";
        }
        return true;
    }
    if (board.isDraw()) {
        result = "1/2-1/2";
        std::cout << "\nDraw (50-move rule or repetition).\n";
        return true;
    }
    return false;
}

void CLI::printStatus() {
    board.print(boardFlipped);
    // Print move history
    if (!sanHistory.empty()) {
        std::cout << "Moves: ";
        for (int i=0;i<(int)sanHistory.size();i++) {
            if (i%2==0) std::cout<<(i/2+1)<<". ";
            std::cout<<sanHistory[i]<<" ";
        }
        std::cout<<"\n\n";
    }
}

void CLI::aiTurn(Color side) {
    std::cout << (side==WHITE?"White":"Black") << " AI thinking for " << aiTime << "s...\n";
    auto start = std::chrono::steady_clock::now();
    
    Move m = search.findBestMove(board, aiTime);
    
    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    
    if (m.isNull()) return;
    
    std::string san = PGN::moveToSAN(board, m);
    board.makeMove(m);
    sanHistory.push_back(san);
    
    int score = search.lastScore;
    if (side==BLACK) score=-score;

    std::string scoreStr = formatScore(score);
    std::cout << "Best move: " << san
              << " (nodes: " << search.nodesSearched
              << ", depth: " << search.depthReached
              << ", eval: " << scoreStr
              << ", time: " << (int)(elapsed*10)/10.0 << "s)\n\n";
}

void CLI::humanTurn() {
    std::string input;
    while (true) {
        std::cout << "Your move: ";
        std::getline(std::cin, input);
        // trim whitespace
        while (!input.empty() && isspace((unsigned char)input.front())) input.erase(input.begin());
        while (!input.empty() && isspace((unsigned char)input.back())) input.pop_back();
        if (input.empty()) continue;

        std::string low = input;
        std::transform(low.begin(),low.end(),low.begin(),::tolower);

        if (low=="quit"||low=="exit") { exit(0); }
        if (low=="undo") { handleCommand("undo"); continue; }
        if (low=="eval") { handleCommand("eval"); continue; }
        if (low=="flip") { handleCommand("flip"); continue; }
        if (low.substr(0,7)=="savepgn") { handleCommand(input); continue; }
        if (low.substr(0,5)=="perft") { handleCommand(input); continue; }

        // Find all legal moves whose SAN matches the input
        auto legal = MoveGen::generateLegalMoves(board);
        std::vector<Move> matches;
        // Strip check/mate from input for comparison
        std::string inp2 = input;
        while (!inp2.empty() && (inp2.back()=='+'||inp2.back()=='#')) inp2.pop_back();

        for (auto& lm : legal) {
            std::string s = PGN::moveToSAN(board, lm);
            while (!s.empty() && (s.back()=='+'||s.back()=='#')) s.pop_back();
            if (s == inp2) matches.push_back(lm);
        }

        // Fallback: try the SAN parser (handles user-supplied disambiguation)
        if (matches.empty()) {
            Move m = PGN::sanToMove(board, input);
            if (!m.isNull()) matches.push_back(m);
        }

        if (matches.empty()) {
            std::cout << "Invalid move. Try again.\n";
            continue;
        }
        if (matches.size() > 1) {
            std::cout << "Ambiguous move. Did you mean:\n";
            for (auto& m : matches) {
                Square from = m.from();
                std::cout << "  " << PGN::moveToSAN(board, m)
                          << " (piece on " << (char)('a'+from%8) << (from/8+1) << ")\n";
            }
            std::cout << "Please be more specific (e.g. include the source file or rank).\n";
            continue;
        }

        Move m = matches[0];
        std::string san = PGN::moveToSAN(board, m);
        if (board.makeMove(m)) {
            sanHistory.push_back(san);
            break;
        } else {
            std::cout << "Illegal move. Try again.\n";
        }
    }
}

void CLI::handleCommand(const std::string& cmd) {
    std::istringstream ss(cmd);
    std::string word; ss >> word;
    std::transform(word.begin(),word.end(),word.begin(),::tolower);
    
    if (word=="flip") {
        boardFlipped = !boardFlipped;
        std::cout << "Board flipped. Now showing from " << (boardFlipped ? "Black" : "White") << "'s perspective.\n";
        printStatus();
    } else if (word=="undo") {
        int steps = (mode==HUMAN_VS_AI) ? 2 : 1;
        for (int i=0;i<steps&&!sanHistory.empty();i++) {
            board.unmakeMove();
            sanHistory.pop_back();
        }
        std::cout << "Move(s) undone.\n";
        printStatus();
    } else if (word=="eval") {
        int score = Eval::evaluate(board); // always White-positive
        std::string scoreStr = formatScore(score);
        const int CM = Eval::CHECKMATE;
        std::cout << "Current evaluation: " << scoreStr;
        if (score > CM - 300) std::cout << " (White has forced mate)";
        else if (score < -(CM - 300)) std::cout << " (Black has forced mate)";
        else if (score > 50) std::cout << " cp (White better)";
        else if (score < -50) std::cout << " cp (Black better)";
        else std::cout << " cp (roughly equal)";
        std::cout << "\nGame phase: " << Eval::gamePhase(board) << "/24\n";
    } else if (word=="savepgn") {
        std::string fname; ss >> fname;
        if (fname.empty()) fname = "game.pgn";
        std::string white = (mode==HUMAN_VS_AI) ? (humanColor==WHITE?"Human":"AI") : "AI-White";
        std::string black = (mode==HUMAN_VS_AI) ? (humanColor==BLACK?"Human":"AI") : "AI-Black";
        auto pgn = PGN::exportPGN(sanHistory, "*", white, black);
        if (PGN::savePGN(pgn, fname)) std::cout << "PGN saved to " << fname << "\n";
        else std::cout << "Failed to save PGN.\n";
    } else if (word=="perft") {
        int depth=1; ss>>depth;
        std::cout << "Perft(" << depth << ")...\n";
        auto start=std::chrono::steady_clock::now();
        uint64_t nodes=MoveGen::perft(board,depth);
        double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout << "Nodes: " << nodes << " (" << (int)(nodes/t) << " nps, " << t << "s)\n";
    }
}

void CLI::gameLoop() {
    printStatus();
    std::string result;

    while (true) {
        if (gameOver(result)) {
            // Offer PGN export
            std::cout << "Save PGN? (filename or 'n'): ";
            std::string fn; std::cin>>fn;
            if (fn!="n"&&fn!="N") {
                std::string white=(mode==HUMAN_VS_AI)?(humanColor==WHITE?"Human":"AI"):"AI-White";
                std::string black=(mode==HUMAN_VS_AI)?(humanColor==BLACK?"Human":"AI"):"AI-Black";
                auto pgn=PGN::exportPGN(sanHistory,result,white,black);
                PGN::savePGN(pgn,fn);
                std::cout<<"Saved to "<<fn<<"\n";
            }
            break;
        }

        Color turn = board.sideToMove();

        if (mode==AI_VS_AI) {
            aiTurn(turn);
            printStatus();
            // Small pause for readability
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } else {
            if (turn==humanColor) {
                humanTurn();
            } else {
                aiTurn(turn);
            }
            printStatus();
        }
    }
}
