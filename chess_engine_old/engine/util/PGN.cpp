#include "PGN.h"
#include "../movegen/MoveGen.h"
#include <sstream>
#include <fstream>
#include <ctime>
#include <iostream>
#include <algorithm>

static const char* fileChar = "abcdefgh";
static const char* rankChar = "12345678";
static const char* pieceChar = ".PNBRQK";

static std::string squareName(Square s) {
    std::string r;
    r += fileChar[s%8];
    r += rankChar[s/8];
    return r;
}

std::string PGN::moveToSAN(Board& board, Move m) {
    if (m.isNull()) return "??";
    
    Square from = m.from(), to = m.to();
    int pc = board.pieceAt(from);
    Piece pt = pieceType(pc);
    Color c = board.sideToMove();
    
    std::string san;

    // Castling
    if (m.flags() == FLAG_CASTLE) {
        if (to%8 == 6) return "O-O";
        else return "O-O-O";
    }

    // Piece letter
    if (pt != PAWN) {
        san += pieceChar[(int)pt];
    }

    // Disambiguation
    if (pt != PAWN) {
        auto legalMoves = MoveGen::generateLegalMoves(board);
        bool ambigFile=false, ambigRank=false, ambig=false;
        for (auto& lm : legalMoves) {
            if (lm==m) continue;
            if (lm.to()!=to) continue;
            int lpc=board.pieceAt(lm.from());
            if (pieceType(lpc)!=pt||pieceColor(lpc)!=c) continue;
            ambig=true;
            if (lm.from()%8==from%8) ambigFile=true;
            else ambigRank=true;
        }
        if (ambig) {
            if (!ambigFile) san+=fileChar[from%8];
            else if (!ambigRank) san+=rankChar[from/8];
            else { san+=fileChar[from%8]; san+=rankChar[from/8]; }
        }
    } else {
        // Pawn: if capture, add file
        if (board.pieceAt(to) || m.flags()==FLAG_EP) {
            san+=fileChar[from%8];
        }
    }

    // Capture
    if (board.pieceAt(to) || m.flags()==FLAG_EP) san+='x';

    // Destination
    san += squareName(to);

    // Promotion
    if (m.flags()==FLAG_PROMO) {
        static const char* promoChars="NBRQ";
        san += '=';
        san += promoChars[m.promo()];
    }

    // Check/checkmate indicator
    if (board.makeMove(m)) {
        bool inCheck = board.isInCheck(board.sideToMove());
        auto resp = MoveGen::generateLegalMoves(board);
        if (inCheck && resp.empty()) san+='#';
        else if (inCheck) san+='+';
        board.unmakeMove();
    }

    return san;
}

Move PGN::sanToMove(Board& board, const std::string& san) {
    if (san.empty()) return Move();
    
    auto legal = MoveGen::generateLegalMoves(board);
    
    // Castling
    if (san=="O-O"||san=="O-O+"||san=="O-O#"||san=="0-0") {
        Color c=board.sideToMove();
        Square from=(c==WHITE)?4:60, to=(c==WHITE)?6:62;
        for (auto& m:legal) if (m.flags()==FLAG_CASTLE&&m.to()==to) return m;
    }
    if (san=="O-O-O"||san=="O-O-O+"||san=="O-O-O#"||san=="0-0-0") {
        Color c=board.sideToMove();
        Square to=(c==WHITE)?2:58;
        for (auto& m:legal) if (m.flags()==FLAG_CASTLE&&m.to()==to) return m;
    }

    std::string s=san;
    // Strip check/mate symbols
    while (!s.empty()&&(s.back()=='+'||s.back()=='#')) s.pop_back();

    // Parse promotion
    int promoType=-1;
    if (s.size()>=2&&s[s.size()-2]=='=') {
        char pc=s.back(); s=s.substr(0,s.size()-2);
        if (pc=='Q') promoType=PROMO_Q;
        else if (pc=='R') promoType=PROMO_R;
        else if (pc=='B') promoType=PROMO_B;
        else if (pc=='N') promoType=PROMO_N;
    }

    // Parse piece type
    Piece pt=PAWN;
    int start=0;
    if (!s.empty()&&isupper(s[0])&&s[0]!='P') {
        switch(s[0]) {
            case 'N': pt=KNIGHT; break; case 'B': pt=BISHOP; break;
            case 'R': pt=ROOK; break; case 'Q': pt=QUEEN; break;
            case 'K': pt=KING; break;
        }
        start=1;
    }

    s=s.substr(start);
    // Remove 'x'
    s.erase(std::remove(s.begin(),s.end(),'x'),s.end());

    // Now s is: [fromFile][fromRank][toFile][toRank] or [fromFile][toFile][toRank] or [toFile][toRank]
    // Last two chars are always destination square
    if (s.size()<2) return Move();
    int toFile=s[s.size()-2]-'a';
    int toRank=s[s.size()-1]-'1';
    if (toFile<0||toFile>7||toRank<0||toRank>7) return Move();
    Square to=toRank*8+toFile;

    // From disambiguation
    int fromFile=-1,fromRank=-1;
    std::string disamB=s.substr(0,s.size()-2);
    for (char c:disamB) {
        if (c>='a'&&c<='h') fromFile=c-'a';
        else if (c>='1'&&c<='8') fromRank=c-'1';
    }

    for (auto& m:legal) {
        if (m.to()!=to) continue;
        int pc=board.pieceAt(m.from());
        if (pieceType(pc)!=pt) continue;
        if (fromFile>=0&&m.from()%8!=fromFile) continue;
        if (fromRank>=0&&m.from()/8!=fromRank) continue;
        if (promoType>=0&&m.flags()==FLAG_PROMO&&m.promo()!=promoType) continue;
        if (promoType>=0&&m.flags()!=FLAG_PROMO) continue;
        if (promoType<0&&m.flags()==FLAG_PROMO) continue;
        return m;
    }
    return Move(); // not found
}

std::string PGN::exportPGN(const std::vector<std::string>& sanMoves,
                              const std::string& result,
                              const std::string& white,
                              const std::string& black,
                              const std::string& event) {
    time_t t=time(nullptr);
    struct tm* tm=localtime(&t);
    char date[20];
    strftime(date,sizeof(date),"%Y.%m.%d",tm);

    std::ostringstream pgn;
    pgn << "[Event \"" << event << "\"]\n";
    pgn << "[Site \"Local\"]\n";
    pgn << "[Date \"" << date << "\"]\n";
    pgn << "[Round \"1\"]\n";
    pgn << "[White \"" << white << "\"]\n";
    pgn << "[Black \"" << black << "\"]\n";
    pgn << "[Result \"" << result << "\"]\n\n";

    for (int i=0;i<(int)sanMoves.size();i++) {
        if (i%2==0) pgn << (i/2+1) << ". ";
        pgn << sanMoves[i] << " ";
    }
    pgn << result << "\n";
    return pgn.str();
}

bool PGN::savePGN(const std::string& pgn, const std::string& filename) {
    std::ofstream f(filename);
    if (!f.is_open()) return false;
    f << pgn;
    return true;
}
