#pragma once
#include "../board/Board.h"
#include "../search/Search.h"
#include "../util/PGN.h"
#include <string>
#include <vector>

enum GameMode { HUMAN_VS_AI, AI_VS_AI };

class CLI {
public:
    CLI();
    void run();

private:
    Board board;
    Search search;
    GameMode mode;
    Color humanColor;
    double aiTime;
    std::vector<std::string> sanHistory;
    std::vector<std::string> undoSanHistory;

    void showMenu();
    void selectMode();
    void gameLoop();
    void humanTurn();
    void aiTurn(Color side);
    void printStatus();
    void handleCommand(const std::string& cmd);
    std::string getResult() const;
    bool gameOver(std::string& result);
};
