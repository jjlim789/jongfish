#include "Board.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <random>
#include <cassert>

// Static initializers
uint64_t Board::zKeys[13][64];
uint64_t Board::zSide;
uint64_t Board::zCastle[16];
uint64_t Board::zEP[8];
bool Board::zInitialized = false;

void Board::initZobrist() {
    if (zInitialized) return;
    std::mt19937_64 rng(0xDEADBEEFCAFEBABEULL);
    for (int p=0;p<13;p++) for (int s=0;s<64;s++) zKeys[p][s]=rng();
    zSide = rng();
    for (int i=0;i<16;i++) zCastle[i]=rng();
    for (int i=0;i<8;i++) zEP[i]=rng();
    zInitialized = true;
}

Board::Board() {
    initZobrist();
    loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static int charToPiece(char c) {
    switch(c) {
        case 'P': return 1; case 'N': return 2; case 'B': return 3;
        case 'R': return 4; case 'Q': return 5; case 'K': return 6;
        case 'p': return 7; case 'n': return 8; case 'b': return 9;
        case 'r': return 10; case 'q': return 11; case 'k': return 12;
    }
    return 0;
}

static char pieceToChar(int pc) {
    const char* s = ".PNBRQKpnbrqk";
    return s[pc];
}

void Board::loadFEN(const std::string& fen) {
    state = BoardState{};
    history.clear();
    stateHistory.clear();
    std::istringstream ss(fen);
    std::string board, side, castle, ep, hm, fm;
    ss >> board >> side >> castle >> ep >> hm >> fm;

    // Board
    int sq = 56; // a8
    for (char c : board) {
        if (c == '/') { sq -= 16; }
        else if (c >= '1' && c <= '8') { sq += c-'0'; }
        else { state.squares[sq] = charToPiece(c); sq++; }
    }
    state.sideToMove = (side == "w") ? WHITE : BLACK;
    state.castling = 0;
    for (char c : castle) {
        if (c=='K') state.castling|=1;
        if (c=='Q') state.castling|=2;
        if (c=='k') state.castling|=4;
        if (c=='q') state.castling|=8;
    }
    if (ep != "-") {
        int f = ep[0]-'a', r = ep[1]-'1';
        state.epSquare = r*8+f;
    }
    state.halfmove = hm.empty() ? 0 : std::stoi(hm);
    state.fullmove = fm.empty() ? 1 : std::stoi(fm);
    recomputeZobrist();
}

std::string Board::toFEN() const {
    std::string fen;
    for (int r=7;r>=0;r--) {
        int empty=0;
        for (int f=0;f<8;f++) {
            int pc = state.squares[r*8+f];
            if (pc==0) empty++;
            else { if(empty){fen+=('0'+empty);empty=0;} fen+=pieceToChar(pc); }
        }
        if (empty) fen+=('0'+empty);
        if (r>0) fen+='/';
    }
    fen += (state.sideToMove==WHITE)?" w ":" b ";
    std::string cas;
    if(state.castling&1) cas+='K';
    if(state.castling&2) cas+='Q';
    if(state.castling&4) cas+='k';
    if(state.castling&8) cas+='q';
    if(cas.empty()) cas="-";
    fen+=cas+" ";
    if(state.epSquare==NO_SQ) fen+="-";
    else { fen+=(char)('a'+state.epSquare%8); fen+=(char)('1'+state.epSquare/8); }
    fen+=" "+std::to_string(state.halfmove)+" "+std::to_string(state.fullmove);
    return fen;
}

void Board::print(bool) const {
    // ASCII: uppercase = White, lowercase = Black
    const char* wPieces[] = {".", "P","N","B","R","Q","K"};
    const char* bPieces[] = {".", "p","n","b","r","q","k"};
    std::cout << "\n  a b c d e f g h\n";
    for (int r=7;r>=0;r--) {
        std::cout << (r+1) << " ";
        for (int f=0;f<8;f++) {
            int pc = state.squares[r*8+f];
            if (pc==0) std::cout << ". ";
            else if (pc<=6) std::cout << wPieces[pc] << " ";
            else std::cout << bPieces[pc-6] << " ";
        }
        std::cout << (r+1) << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << (state.sideToMove==WHITE?"White":"Black") << " to move";
    if (state.epSquare!=NO_SQ) std::cout << " | EP: " << (char)('a'+state.epSquare%8) << (state.epSquare/8+1);
    std::cout << " | Castling: ";
    if(state.castling&1) std::cout<<"K";
    if(state.castling&2) std::cout<<"Q";
    if(state.castling&4) std::cout<<"k";
    if(state.castling&8) std::cout<<"q";
    if(!state.castling) std::cout<<"-";
    std::cout << "\n\n";
}

void Board::setPiece(Square s, int pc) {
    state.zobrist ^= zKeys[state.squares[s]][s];
    state.squares[s] = pc;
    state.zobrist ^= zKeys[pc][s];
}

void Board::clearPiece(Square s) {
    state.zobrist ^= zKeys[state.squares[s]][s];
    state.squares[s] = 0;
    state.zobrist ^= zKeys[0][s];
}

void Board::recomputeZobrist() {
    state.zobrist = 0;
    for (int s=0;s<64;s++) state.zobrist ^= zKeys[state.squares[s]][s];
    if (state.sideToMove==BLACK) state.zobrist ^= zSide;
    state.zobrist ^= zCastle[state.castling];
    if (state.epSquare!=NO_SQ) state.zobrist ^= zEP[state.epSquare%8];
}

bool Board::isSquareAttacked(Square s, Color byColor) const {
    // Check if square s is attacked by byColor
    // King attacks
    int koff[] = {-9,-8,-7,-1,1,7,8,9};
    for (int o : koff) {
        int t = s+o;
        if (t<0||t>63) continue;
        if (abs((t%8)-(s%8))>1) continue;
        if (abs((t/8)-(s/8))>1) continue;
        int pc = state.squares[t];
        if (pc && pieceColor(pc)==byColor && pieceType(pc)==KING) return true;
    }
    // Knight
    int noff[] = {-17,-15,-10,-6,6,10,15,17};
    for (int o : noff) {
        int t = s+o;
        if (t<0||t>63) continue;
        int df = abs((t%8)-(s%8));
        if (df!=1&&df!=2) continue;
        int pc = state.squares[t];
        if (pc && pieceColor(pc)==byColor && pieceType(pc)==KNIGHT) return true;
    }
    // Pawn attacks
    if (byColor==WHITE) {
        int t1=s-7, t2=s-9;
        if (t1>=0 && t1<64 && abs((t1%8)-(s%8))==1) {
            if (state.squares[t1]==makePiece(WHITE,PAWN)) return true;
        }
        if (t2>=0 && t2<64 && abs((t2%8)-(s%8))==1) {
            if (state.squares[t2]==makePiece(WHITE,PAWN)) return true;
        }
    } else {
        int t1=s+7, t2=s+9;
        if (t1>=0 && t1<64 && abs((t1%8)-(s%8))==1) {
            if (state.squares[t1]==makePiece(BLACK,PAWN)) return true;
        }
        if (t2>=0 && t2<64 && abs((t2%8)-(s%8))==1) {
            if (state.squares[t2]==makePiece(BLACK,PAWN)) return true;
        }
    }
    // Sliding pieces
    // Rook/Queen (horizontal/vertical)
    int rdirs[] = {1,-1,8,-8};
    for (int d : rdirs) {
        int t = s+d;
        while (t>=0&&t<64) {
            if (d==1&&t%8==0) break;
            if (d==-1&&t%8==7) break;
            int pc = state.squares[t];
            if (pc) {
                if (pieceColor(pc)==byColor && (pieceType(pc)==ROOK||pieceType(pc)==QUEEN)) return true;
                break;
            }
            t+=d;
        }
    }
    // Bishop/Queen (diagonal)
    int bdirs[] = {9,-9,7,-7};
    for (int d : bdirs) {
        int t = s+d;
        while (t>=0&&t<64) {
            int tf=t%8, sf=s%8;
            if (abs(tf-sf)!=abs((t/8)-(s/8))) break; // off board diagonally
            int pc = state.squares[t];
            if (pc) {
                if (pieceColor(pc)==byColor && (pieceType(pc)==BISHOP||pieceType(pc)==QUEEN)) return true;
                break;
            }
            t+=d;
        }
    }
    return false;
}

bool Board::isInCheck(Color c) const {
    // Find king
    int kpc = makePiece(c, KING);
    Square ks = -1;
    for (int s=0;s<64;s++) if (state.squares[s]==kpc) { ks=s; break; }
    if (ks==-1) return false;
    return isSquareAttacked(ks, c==WHITE?BLACK:WHITE);
}

bool Board::makeMove(Move m) {
    stateHistory.push_back(state);
    
    Square from = m.from(), to = m.to();
    int flags = m.flags();
    
    state.prevCastling = state.castling;
    state.prevEP = state.epSquare;
    state.prevHalfmove = state.halfmove;
    state.capturedPiece = 0;
    state.lastMove = m;

    // Update zobrist for old ep/castle
    if (state.epSquare!=NO_SQ) state.zobrist ^= zEP[state.epSquare%8];
    state.zobrist ^= zCastle[state.castling];
    
    int pc = state.squares[from];
    int cap = state.squares[to];
    state.capturedPiece = cap;

    // Halfmove
    if (cap || pieceType(pc)==PAWN) state.halfmove=0;
    else state.halfmove++;

    state.epSquare = NO_SQ;

    if (flags==FLAG_EP) {
        // En passant
        Square capSq = to + (state.sideToMove==WHITE?-8:8);
        state.capturedPiece = state.squares[capSq];
        state.zobrist ^= zKeys[state.squares[capSq]][capSq];
        state.squares[capSq] = 0;
        state.zobrist ^= zKeys[0][capSq];
    } else if (flags==FLAG_CASTLE) {
        // Move rook too
        int rf, rt;
        if (to==6||to==62) { // Kingside
            rf = (state.sideToMove==WHITE)?7:63;
            rt = (state.sideToMove==WHITE)?5:61;
        } else { // Queenside
            rf = (state.sideToMove==WHITE)?0:56;
            rt = (state.sideToMove==WHITE)?3:59;
        }
        int rpc = state.squares[rf];
        state.zobrist ^= zKeys[rpc][rf];
        state.squares[rf] = 0;
        state.zobrist ^= zKeys[0][rf];
        state.zobrist ^= zKeys[0][rt];
        state.squares[rt] = rpc;
        state.zobrist ^= zKeys[rpc][rt];
    }

    // Move piece
    state.zobrist ^= zKeys[pc][from];
    state.squares[from] = 0;
    state.zobrist ^= zKeys[0][from];
    
    if (flags==FLAG_PROMO) {
        int promoPiece[] = {KNIGHT,BISHOP,ROOK,QUEEN};
        int npc = makePiece(state.sideToMove, (Piece)promoPiece[m.promo()]);
        state.zobrist ^= zKeys[cap][to];
        state.squares[to] = npc;
        state.zobrist ^= zKeys[npc][to];
    } else {
        state.zobrist ^= zKeys[cap][to];
        state.squares[to] = pc;
        state.zobrist ^= zKeys[pc][to];
    }

    // En passant square
    if (pieceType(pc)==PAWN && abs(to-from)==16) {
        state.epSquare = (from+to)/2;
        state.zobrist ^= zEP[state.epSquare%8];
    }

    // Update castling rights
    // Remove castling rights if king or rook moves
    if (from==4||to==4) state.castling&=~3; // white king
    if (from==60||to==60) state.castling&=~12; // black king
    if (from==0||to==0) state.castling&=~2; // white QR
    if (from==7||to==7) state.castling&=~1; // white KR
    if (from==56||to==56) state.castling&=~8; // black QR
    if (from==63||to==63) state.castling&=~4; // black KR
    
    state.zobrist ^= zCastle[state.castling];

    // Side to move
    state.sideToMove = (state.sideToMove==WHITE)?BLACK:WHITE;
    state.zobrist ^= zSide;
    if (state.sideToMove==WHITE) state.fullmove++;

    // Check if move leaves us in check (illegal)
    Color mover = (state.sideToMove==WHITE)?BLACK:WHITE;
    if (isInCheck(mover)) {
        unmakeMove();
        return false;
    }

    history.push_back(m);
    return true;
}

void Board::unmakeMove() {
    if (stateHistory.empty()) return;
    // pop history
    if (!history.empty()) history.pop_back();
    state = stateHistory.back();
    stateHistory.pop_back();
}

int Board::repetitionCount() const {
    int cnt = 1;
    uint64_t z = state.zobrist;
    for (int i=(int)stateHistory.size()-1;i>=0;i--) {
        if (stateHistory[i].zobrist==z) cnt++;
        if (stateHistory[i].halfmove==0 && i!=(int)stateHistory.size()-1) break;
    }
    return cnt;
}

bool Board::isDraw() const {
    if (state.halfmove>=100) return true;
    if (repetitionCount()>=3) return true;
    // Insufficient material
    int wn=countPiece(WHITE,KNIGHT),bn=countPiece(BLACK,KNIGHT);
    int wb=countPiece(WHITE,BISHOP),bb=countPiece(BLACK,BISHOP);
    int wr=countPiece(WHITE,ROOK),br=countPiece(BLACK,ROOK);
    int wq=countPiece(WHITE,QUEEN),bq=countPiece(BLACK,QUEEN);
    int wp=countPiece(WHITE,PAWN),bp=countPiece(BLACK,PAWN);
    if (wp||bp||wr||br||wq||bq) return false;
    if (wn==0&&wb==0&&bn==0&&bb==0) return true; // K vs K
    if (wb==1&&wn==0&&bb==0&&bn==0&&wp==0&&bp==0) return true; // K+B vs K
    if (bb==1&&bn==0&&wb==0&&wn==0&&wp==0&&bp==0) return true;
    if (wn==1&&wb==0&&bb==0&&bn==0&&wp==0&&bp==0) return true;
    if (bn==1&&bb==0&&wb==0&&wn==0&&wp==0&&bp==0) return true;
    return false;
}

int Board::countPiece(Color c, Piece p) const {
    int pc = makePiece(c,p);
    int cnt=0;
    for (int s=0;s<64;s++) if (state.squares[s]==pc) cnt++;
    return cnt;
}

Bitboard Board::pieceBB(Color c, Piece p) const {
    int pc = makePiece(c,p);
    Bitboard bb=0;
    for (int s=0;s<64;s++) if (state.squares[s]==pc) bb|=(1ULL<<s);
    return bb;
}

bool Board::isCheckmate() {
    if (!isInCheck(state.sideToMove)) return false;
    // Need movegen - will use from CLI
    return false; // placeholder, CLI handles this
}

bool Board::isStalemate() {
    if (isInCheck(state.sideToMove)) return false;
    return false; // placeholder
}
