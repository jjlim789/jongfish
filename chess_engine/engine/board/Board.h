#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>

// Piece types
enum Piece { NONE=0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color { WHITE=0, BLACK=1 };

// Squares 0-63, a1=0, h8=63
using Square = int;
using Bitboard = uint64_t;

constexpr Square NO_SQ = -1;

// Piece encoding: 0=none, 1-6 white P/N/B/R/Q/K, 7-12 black P/N/B/R/Q/K
inline int makePiece(Color c, Piece p) { return (int)p + (c == BLACK ? 6 : 0); }
inline Color pieceColor(int pc) { return (pc >= 7) ? BLACK : WHITE; }
inline Piece pieceType(int pc) { return (Piece)(pc > 6 ? pc - 6 : pc); }

// Move encoding: 16 bits
// bits 0-5: from, 6-11: to, 12-13: promo (0=none,1=N,2=B,3=R,4=Q), 14-15: flags
// flags: 0=normal,1=castle,2=ep,3=promo
struct Move {
    uint16_t data = 0;
    Move() = default;
    Move(Square from, Square to, int flags=0, int promo=0) {
        data = (uint16_t)(from | (to<<6) | (flags<<12) | (promo<<14));
    }
    Square from() const { return data & 63; }
    Square to() const { return (data >> 6) & 63; }
    int flags() const { return (data >> 12) & 3; }
    int promo() const { return (data >> 14) & 3; }
    bool isNull() const { return data == 0; }
    bool operator==(const Move& o) const { return data == o.data; }
    bool operator!=(const Move& o) const { return data != o.data; }
};

constexpr int FLAG_NORMAL = 0;
constexpr int FLAG_CASTLE = 1;
constexpr int FLAG_EP = 2;
constexpr int FLAG_PROMO = 3;

// Promo piece encoding in move
constexpr int PROMO_N = 0;
constexpr int PROMO_B = 1;
constexpr int PROMO_R = 2;
constexpr int PROMO_Q = 3;

struct BoardState {
    // 0=none,1=wP,2=wN,3=wB,4=wR,5=wQ,6=wK,7=bP,8=bN,9=bB,10=bR,11=bQ,12=bK
    std::array<int,64> squares{};
    // Castling rights: bit0=wK, bit1=wQ, bit2=bK, bit3=bQ
    int castling = 0;
    Square epSquare = NO_SQ;
    int halfmove = 0;
    int fullmove = 1;
    Color sideToMove = WHITE;
    uint64_t zobrist = 0;
    Move lastMove;
    int capturedPiece = 0; // for unmake
    int prevCastling = 0;
    Square prevEP = NO_SQ;
    int prevHalfmove = 0;
};

class Board {
public:
    Board();
    void loadFEN(const std::string& fen);
    std::string toFEN() const;
    void print(bool flipped = false) const;

    bool makeMove(Move m);   // returns false if illegal (leaves in check)
    void unmakeMove();

    std::vector<Move> history;
    std::vector<BoardState> stateHistory;

    // Current state accessors
    int pieceAt(Square s) const { return state.squares[s]; }
    Color sideToMove() const { return state.sideToMove; }
    int castlingRights() const { return state.castling; }
    Square epSquare() const { return state.epSquare; }
    int halfmove() const { return state.halfmove; }
    int fullmove() const { return state.fullmove; }
    uint64_t zobrist() const { return state.zobrist; }

    bool isInCheck(Color c) const;
    bool isSquareAttacked(Square s, Color byColor) const;
    bool isDraw() const; // 50-move, repetition
    bool isCheckmate();
    bool isStalemate();

    // For evaluation
    int countPiece(Color c, Piece p) const;
    Bitboard pieceBB(Color c, Piece p) const; // returns bitboard of piece locations

    const BoardState& getState() const { return state; }

private:
    BoardState state;
    // Zobrist keys
    static uint64_t zKeys[13][64];
    static uint64_t zSide;
    static uint64_t zCastle[16];
    static uint64_t zEP[8];
    static bool zInitialized;
    static void initZobrist();
    void recomputeZobrist();

    void setPiece(Square s, int pc);
    void clearPiece(Square s);
    int repetitionCount() const;
};
