#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <utility>
#include <limits>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <cstdint>

using namespace std;

enum PieceType { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color { NONE, WHITE, BLACK };

// Forward declaration
class ChessBoard;

// Structure to represent a chess move
struct ChessMove {
    int fromRow, fromCol, toRow, toCol;
    PieceType promotionPiece;
    
    ChessMove() : fromRow(-1), fromCol(-1), toRow(-1), toCol(-1), promotionPiece(EMPTY) {}
    ChessMove(int fr, int fc, int tr, int tc, PieceType promo = EMPTY) 
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), promotionPiece(promo) {}
    
    string toAlgebraic() const;
    string toSAN(const class ChessBoard& board) const;  // Standard Algebraic Notation
    bool isValid() const { return fromRow >= 0 && fromRow < 8 && toRow >= 0 && toRow < 8; }
};

// Abstract AI interface
class ChessAI {
public:
    virtual ~ChessAI() {}
    virtual ChessMove getBestMove(const ChessBoard& board, Color color) = 0;
    virtual string getName() const = 0;
    virtual bool usesTimeLimit() const { return false; }  // Does this AI use time limits?
    virtual void setTimeLimit(double seconds) {}  // Set time limit in seconds
    virtual void setDepth(int depth) {}  // Set search depth
};

struct Piece {
    PieceType type;
    Color color;
    
    Piece() : type(EMPTY), color(NONE) {}
    Piece(PieceType t, Color c) : type(t), color(c) {}
    
    char getSymbol() const {
        if (type == EMPTY) return '.';
        
        char symbol;
        switch(type) {
            case PAWN: symbol = 'P'; break;
            case KNIGHT: symbol = 'N'; break;
            case BISHOP: symbol = 'B'; break;
            case ROOK: symbol = 'R'; break;
            case QUEEN: symbol = 'Q'; break;
            case KING: symbol = 'K'; break;
            default: symbol = '.';
        }
        
        return (color == WHITE) ? symbol : tolower(symbol);
    }
};

struct Move {
    int fromRow, fromCol, toRow, toCol;
    Piece capturedPiece;
    bool wasEnPassant;
    int prevEnPassantCol;
    bool wasCastling;
    double evaluation;  // Store AI evaluation (white perspective: + = white winning, - = black winning)
    string sanNotation;  // Standard Algebraic Notation
    int halfMoveClock;  // For fifty-move rule
    
    Move(int fr, int fc, int tr, int tc, Piece cap, bool ep, int epCol, bool castle = false, double eval = 0.0, string san = "", int hmc = 0) 
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), capturedPiece(cap), 
          wasEnPassant(ep), prevEnPassantCol(epCol), wasCastling(castle), evaluation(eval), sanNotation(san), halfMoveClock(hmc) {}
};

// Position hash for repetition detection
class PositionHash {
public:
    static string hashPosition(const class ChessBoard& board);
};

class ChessBoard {
private:
    Piece board[8][8];
    Color currentTurn;
    bool flipped;
    int enPassantCol;  // -1 if no en passant available, otherwise the column of the pawn that can be captured
    vector<Move> moveHistory;
    map<string, int> positionHistory;  // For threefold repetition detection
    int halfMoveClock;  // For fifty-move rule (resets on pawn move or capture)
    
    // Castling rights
    bool whiteKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackKingMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;
    
public:
    ChessBoard() : currentTurn(WHITE), flipped(false), enPassantCol(-1), halfMoveClock(0),
                   whiteKingMoved(false), whiteRookKingsideMoved(false), whiteRookQueensideMoved(false),
                   blackKingMoved(false), blackRookKingsideMoved(false), blackRookQueensideMoved(false) {
        initializeBoard();
        // Initialize position history with starting position
        positionHistory[PositionHash::hashPosition(*this)] = 1;
    }
    
    void initializeBoard() {
        // Clear board
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board[i][j] = Piece();
            }
        }
        
        // Set up pawns
        for (int j = 0; j < 8; j++) {
            board[1][j] = Piece(PAWN, BLACK);
            board[6][j] = Piece(PAWN, WHITE);
        }
        
        // Set up back ranks
        PieceType backRank[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
        for (int j = 0; j < 8; j++) {
            board[0][j] = Piece(backRank[j], BLACK);
            board[7][j] = Piece(backRank[j], WHITE);
        }
    }
    
    void display() const {
        cout << "\n";
        
        if (!flipped) {
            // Normal view (White on bottom)
            for (int i = 0; i < 8; i++) {
                cout << (8 - i) << " ";
                for (int j = 0; j < 8; j++) {
                    cout << board[i][j].getSymbol() << " ";
                }
                cout << "\n";
            }
            cout << "  a b c d e f g h\n";
        } else {
            // Flipped view (Black on bottom)
            for (int i = 7; i >= 0; i--) {
                cout << (8 - i) << " ";
                for (int j = 7; j >= 0; j--) {
                    cout << board[i][j].getSymbol() << " ";
                }
                cout << "\n";
            }
            cout << "  h g f e d c b a\n";
        }
        
        cout << "\n" << (currentTurn == WHITE ? "White" : "Black") << " to move";
        
        if (isCheckmate(currentTurn)) {
            cout << " - CHECKMATE! " << (currentTurn == WHITE ? "Black" : "White") << " wins!";
        } else if (isStalemate(currentTurn)) {
            cout << " - STALEMATE! Draw.";
        } else if (isInCheck(currentTurn)) {
            cout << " - IN CHECK!";
        }
        
        cout << "\n";
    }
    
    bool undoMove() {
        if (moveHistory.empty()) {
            return false;
        }
        
        // Remove current position from history
        string currentHash = PositionHash::hashPosition(*this);
        if (positionHistory[currentHash] > 0) {
            positionHistory[currentHash]--;
            if (positionHistory[currentHash] == 0) {
                positionHistory.erase(currentHash);
            }
        }
        
        Move lastMove = moveHistory.back();
        moveHistory.pop_back();
        
        // Restore half-move clock
        halfMoveClock = lastMove.halfMoveClock;
        
        // Restore the piece to its original position
        board[lastMove.fromRow][lastMove.fromCol] = board[lastMove.toRow][lastMove.toCol];
        
        // If this was a promotion, restore the pawn (not the promoted piece)
        if (board[lastMove.fromRow][lastMove.fromCol].type != PAWN &&
            board[lastMove.fromRow][lastMove.fromCol].type != EMPTY) {
            // Check if this could have been a promotion
            if ((lastMove.fromRow == 1 && lastMove.toRow == 0) ||  // White pawn promotion
                (lastMove.fromRow == 6 && lastMove.toRow == 7)) {  // Black pawn promotion
                board[lastMove.fromRow][lastMove.fromCol].type = PAWN;
            }
        }
        
        board[lastMove.toRow][lastMove.toCol] = lastMove.capturedPiece;
        
        // Undo castling - move the rook back
        if (lastMove.wasCastling) {
            if (lastMove.toCol == 6) {  // Kingside
                board[lastMove.toRow][7] = board[lastMove.toRow][5];
                board[lastMove.toRow][5] = Piece();
            } else if (lastMove.toCol == 2) {  // Queenside
                board[lastMove.toRow][0] = board[lastMove.toRow][3];
                board[lastMove.toRow][3] = Piece();
            }
        }
        
        // Undo en passant
        if (lastMove.wasEnPassant) {
            int capturedPawnRow = (board[lastMove.fromRow][lastMove.fromCol].color == WHITE) ? lastMove.toRow + 1 : lastMove.toRow - 1;
            board[capturedPawnRow][lastMove.toCol] = lastMove.capturedPiece;
            board[lastMove.toRow][lastMove.toCol] = Piece();
        }
        
        // Restore en passant column
        enPassantCol = lastMove.prevEnPassantCol;
        
        // Restore castling rights by checking move history
        whiteKingMoved = false;
        whiteRookKingsideMoved = false;
        whiteRookQueensideMoved = false;
        blackKingMoved = false;
        blackRookKingsideMoved = false;
        blackRookQueensideMoved = false;
        
        for (const Move& m : moveHistory) {
            Piece movedPiece = board[m.toRow][m.toCol];
            if (m.toRow == m.fromRow && m.toCol == m.fromCol) {
                // Retrieve piece from history if needed
                movedPiece = board[m.fromRow][m.fromCol];
            }
            
            // Check what piece was moved in this historical move
            if (m.fromRow == 7 && m.fromCol == 4) {
                whiteKingMoved = true;
            } else if (m.fromRow == 0 && m.fromCol == 4) {
                blackKingMoved = true;
            } else if (m.fromRow == 7 && m.fromCol == 7) {
                whiteRookKingsideMoved = true;
            } else if (m.fromRow == 7 && m.fromCol == 0) {
                whiteRookQueensideMoved = true;
            } else if (m.fromRow == 0 && m.fromCol == 7) {
                blackRookKingsideMoved = true;
            } else if (m.fromRow == 0 && m.fromCol == 0) {
                blackRookQueensideMoved = true;
            }
        }
        
        // Switch turn back
        currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
        
        return true;
    }
    
    bool isSquareAttacked(int row, int col, Color byColor) const {
        // Check if square (row, col) is attacked by any piece of color byColor
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color == byColor && board[r][c].type != EMPTY) {
                    // Temporarily check if this piece can attack the square
                    // We need to use a modified isValidMove that doesn't check for check
                    if (canPieceAttackSquare(r, c, row, col)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    bool canPieceAttackSquare(int fromRow, int fromCol, int toRow, int toCol) const {
        Piece fromPiece = board[fromRow][fromCol];
        Piece toPiece = board[toRow][toCol];
        
        if (fromPiece.type == EMPTY) return false;
        
        int rowDiff = toRow - fromRow;
        int colDiff = toCol - fromCol;
        int absRowDiff = abs(rowDiff);
        int absColDiff = abs(colDiff);
        
        switch(fromPiece.type) {
            case PAWN: {
                int direction = (fromPiece.color == WHITE) ? -1 : 1;
                // Pawns attack diagonally
                if (absColDiff == 1 && rowDiff == direction) {
                    return true;
                }
                return false;
            }
            case KNIGHT:
                return (absRowDiff == 2 && absColDiff == 1) || (absRowDiff == 1 && absColDiff == 2);
            case BISHOP:
                if (absRowDiff != absColDiff) return false;
                return isPathClear(fromRow, fromCol, toRow, toCol);
            case ROOK:
                if (rowDiff != 0 && colDiff != 0) return false;
                return isPathClear(fromRow, fromCol, toRow, toCol);
            case QUEEN:
                if (rowDiff != 0 && colDiff != 0 && absRowDiff != absColDiff) return false;
                return isPathClear(fromRow, fromCol, toRow, toCol);
            case KING:
                return absRowDiff <= 1 && absColDiff <= 1;
            default:
                return false;
        }
    }
    
    pair<int, int> findKing(Color color) const {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].type == KING && board[r][c].color == color) {
                    return {r, c};
                }
            }
        }
        return {-1, -1};  // Should never happen
    }
    
    bool isInCheck(Color color) const {
        pair<int, int> kingPos = findKing(color);
        int kingRow = kingPos.first;
        int kingCol = kingPos.second;
        if (kingRow == -1) return false;
        
        Color opponent = (color == WHITE) ? BLACK : WHITE;
        return isSquareAttacked(kingRow, kingCol, opponent);
    }
    
    bool wouldBeInCheck(int fromRow, int fromCol, int toRow, int toCol, Color color) const {
        // Temporarily make the move and check if king would be in check
        Piece tempFrom = board[fromRow][fromCol];
        Piece tempTo = board[toRow][toCol];
        
        // Make temporary move
        const_cast<ChessBoard*>(this)->board[toRow][toCol] = tempFrom;
        const_cast<ChessBoard*>(this)->board[fromRow][fromCol] = Piece();
        
        bool inCheck = isInCheck(color);
        
        // Undo temporary move
        const_cast<ChessBoard*>(this)->board[fromRow][fromCol] = tempFrom;
        const_cast<ChessBoard*>(this)->board[toRow][toCol] = tempTo;
        
        return inCheck;
    }
    
    bool hasLegalMoves(Color color) const {
        // Check if the given color has any legal moves
        for (int fromR = 0; fromR < 8; fromR++) {
            for (int fromC = 0; fromC < 8; fromC++) {
                if (board[fromR][fromC].color == color && board[fromR][fromC].type != EMPTY) {
                    for (int toR = 0; toR < 8; toR++) {
                        for (int toC = 0; toC < 8; toC++) {
                            if (isValidMove(fromR, fromC, toR, toC)) {
                                if (!wouldBeInCheck(fromR, fromC, toR, toC, color)) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
    
    bool isCheckmate(Color color) const {
        return isInCheck(color) && !hasLegalMoves(color);
    }
    
    bool isStalemate(Color color) const {
        return !isInCheck(color) && !hasLegalMoves(color);
    }
    
    bool isThreefoldRepetition() const {
        string currentHash = PositionHash::hashPosition(*this);
        auto it = positionHistory.find(currentHash);
        return (it != positionHistory.end() && it->second >= 3);
    }
    
    bool isFiftyMoveRule() const {
        return halfMoveClock >= 100;  // 50 moves = 100 half-moves
    }
    
    bool isInsufficientMaterial() const {
        // Count pieces
        int whiteKnights = 0, blackKnights = 0;
        int whiteBishops = 0, blackBishops = 0;
        int whitePawns = 0, blackPawns = 0;
        int whiteRooks = 0, blackRooks = 0;
        int whiteQueens = 0, blackQueens = 0;
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                Piece p = board[row][col];
                if (p.type == EMPTY || p.type == KING) continue;
                
                if (p.color == WHITE) {
                    switch(p.type) {
                        case PAWN: whitePawns++; break;
                        case KNIGHT: whiteKnights++; break;
                        case BISHOP: whiteBishops++; break;
                        case ROOK: whiteRooks++; break;
                        case QUEEN: whiteQueens++; break;
                        default: break;
                    }
                } else {
                    switch(p.type) {
                        case PAWN: blackPawns++; break;
                        case KNIGHT: blackKnights++; break;
                        case BISHOP: blackBishops++; break;
                        case ROOK: blackRooks++; break;
                        case QUEEN: blackQueens++; break;
                        default: break;
                    }
                }
            }
        }
        
        // Any pawns, rooks, or queens = sufficient material
        if (whitePawns > 0 || blackPawns > 0) return false;
        if (whiteRooks > 0 || blackRooks > 0) return false;
        if (whiteQueens > 0 || blackQueens > 0) return false;
        
        int whitePieces = whiteKnights + whiteBishops;
        int blackPieces = blackKnights + blackBishops;
        
        // King vs King
        if (whitePieces == 0 && blackPieces == 0) return true;
        
        // King + minor piece vs King
        if (whitePieces == 1 && blackPieces == 0) return true;
        if (whitePieces == 0 && blackPieces == 1) return true;
        
        // King + Bishop vs King + Bishop (same color bishops)
        if (whiteBishops == 1 && blackBishops == 1 && whiteKnights == 0 && blackKnights == 0) {
            // Find bishop colors
            bool whiteBishopOnLight = false, blackBishopOnLight = false;
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    Piece p = board[row][col];
                    if (p.type == BISHOP) {
                        bool isLightSquare = ((row + col) % 2 == 0);
                        if (p.color == WHITE) whiteBishopOnLight = isLightSquare;
                        else blackBishopOnLight = isLightSquare;
                    }
                }
            }
            if (whiteBishopOnLight == blackBishopOnLight) return true;
        }
        
        return false;
    }
    
    bool isDraw() const {
        return isThreefoldRepetition() || isFiftyMoveRule() || isInsufficientMaterial();
    }
    
    void flipBoard() {
        flipped = !flipped;
    }
    
    bool parseSquare(const string& sq, int& row, int& col) const {
        if (sq.length() != 2) return false;
        
        char file = tolower(sq[0]);
        char rank = sq[1];
        
        if (file < 'a' || file > 'h') return false;
        if (rank < '1' || rank > '8') return false;
        
        col = file - 'a';
        row = 8 - (rank - '0');
        
        return true;
    }
    
    bool isValidMove(int fromRow, int fromCol, int toRow, int toCol) const {
        // Check bounds
        if (fromRow < 0 || fromRow > 7 || fromCol < 0 || fromCol > 7) return false;
        if (toRow < 0 || toRow > 7 || toCol < 0 || toCol > 7) return false;
        
        Piece fromPiece = board[fromRow][fromCol];
        Piece toPiece = board[toRow][toCol];
        
        // Check if there's a piece to move
        if (fromPiece.type == EMPTY) return false;
        
        // Check if it's the right color's turn
        if (fromPiece.color != currentTurn) return false;
        
        // Check if trying to capture own piece
        if (toPiece.type != EMPTY && toPiece.color == currentTurn) return false;
        
        // Basic move validation for each piece type
        int rowDiff = toRow - fromRow;
        int colDiff = toCol - fromCol;
        int absRowDiff = abs(rowDiff);
        int absColDiff = abs(colDiff);
        
        bool isBasicMoveValid = false;
        
        switch(fromPiece.type) {
            case PAWN: {
                int direction = (fromPiece.color == WHITE) ? -1 : 1;
                int startRow = (fromPiece.color == WHITE) ? 6 : 1;
                
                // Forward move
                if (colDiff == 0 && rowDiff == direction && toPiece.type == EMPTY) {
                    isBasicMoveValid = true;
                }
                // Double forward from start
                else if (colDiff == 0 && rowDiff == 2 * direction && fromRow == startRow && 
                    toPiece.type == EMPTY && board[fromRow + direction][fromCol].type == EMPTY) {
                    isBasicMoveValid = true;
                }
                // Capture
                else if (absColDiff == 1 && rowDiff == direction && toPiece.type != EMPTY) {
                    isBasicMoveValid = true;
                }
                // En passant capture
                else if (absColDiff == 1 && rowDiff == direction && toPiece.type == EMPTY) {
                    // Check if this is an en passant capture
                    int enPassantRow = (fromPiece.color == WHITE) ? 3 : 4;
                    if (fromRow == enPassantRow && toCol == enPassantCol) {
                        isBasicMoveValid = true;
                    }
                }
                break;
            }
            
            case KNIGHT:
                isBasicMoveValid = (absRowDiff == 2 && absColDiff == 1) || (absRowDiff == 1 && absColDiff == 2);
                break;
            
            case BISHOP:
                if (absRowDiff == absColDiff) {
                    isBasicMoveValid = isPathClear(fromRow, fromCol, toRow, toCol);
                }
                break;
            
            case ROOK:
                if (rowDiff == 0 || colDiff == 0) {
                    isBasicMoveValid = isPathClear(fromRow, fromCol, toRow, toCol);
                }
                break;
            
            case QUEEN:
                if ((rowDiff == 0 || colDiff == 0) || (absRowDiff == absColDiff)) {
                    isBasicMoveValid = isPathClear(fromRow, fromCol, toRow, toCol);
                }
                break;
            
            case KING:
                if (absRowDiff <= 1 && absColDiff <= 1) {
                    isBasicMoveValid = true;
                }
                // Castling
                else if (absRowDiff == 0 && absColDiff == 2) {
                    isBasicMoveValid = canCastle(fromRow, fromCol, toRow, toCol);
                }
                break;
            
            default:
                isBasicMoveValid = false;
        }
        
        if (!isBasicMoveValid) return false;
        
        // Check if this move would leave the king in check
        if (wouldBeInCheck(fromRow, fromCol, toRow, toCol, currentTurn)) {
            return false;
        }
        
        return true;
    }
    
    bool canCastle(int fromRow, int fromCol, int toRow, int toCol) const {
        Piece king = board[fromRow][fromCol];
        if (king.type != KING) return false;
        
        bool isKingside = (toCol > fromCol);
        
        if (king.color == WHITE) {
            if (fromRow != 7 || fromCol != 4) return false;
            if (whiteKingMoved) return false;
            
            if (isKingside) {
                // Kingside castling
                if (whiteRookKingsideMoved) return false;
                if (toCol != 6) return false;
                // Check squares are empty
                if (board[7][5].type != EMPTY || board[7][6].type != EMPTY) return false;
                // Check king doesn't pass through check
                if (isInCheck(WHITE)) return false;
                if (isSquareAttacked(7, 5, BLACK)) return false;
                if (isSquareAttacked(7, 6, BLACK)) return false;
                return true;
            } else {
                // Queenside castling
                if (whiteRookQueensideMoved) return false;
                if (toCol != 2) return false;
                // Check squares are empty
                if (board[7][1].type != EMPTY || board[7][2].type != EMPTY || board[7][3].type != EMPTY) return false;
                // Check king doesn't pass through check
                if (isInCheck(WHITE)) return false;
                if (isSquareAttacked(7, 3, BLACK)) return false;
                if (isSquareAttacked(7, 2, BLACK)) return false;
                return true;
            }
        } else {
            if (fromRow != 0 || fromCol != 4) return false;
            if (blackKingMoved) return false;
            
            if (isKingside) {
                // Kingside castling
                if (blackRookKingsideMoved) return false;
                if (toCol != 6) return false;
                // Check squares are empty
                if (board[0][5].type != EMPTY || board[0][6].type != EMPTY) return false;
                // Check king doesn't pass through check
                if (isInCheck(BLACK)) return false;
                if (isSquareAttacked(0, 5, WHITE)) return false;
                if (isSquareAttacked(0, 6, WHITE)) return false;
                return true;
            } else {
                // Queenside castling
                if (blackRookQueensideMoved) return false;
                if (toCol != 2) return false;
                // Check squares are empty
                if (board[0][1].type != EMPTY || board[0][2].type != EMPTY || board[0][3].type != EMPTY) return false;
                // Check king doesn't pass through check
                if (isInCheck(BLACK)) return false;
                if (isSquareAttacked(0, 3, WHITE)) return false;
                if (isSquareAttacked(0, 2, WHITE)) return false;
                return true;
            }
        }
    }
    
    bool isPathClear(int fromRow, int fromCol, int toRow, int toCol) const {
        int rowDir = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
        int colDir = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
        
        int r = fromRow + rowDir;
        int c = fromCol + colDir;
        
        while (r != toRow || c != toCol) {
            if (board[r][c].type != EMPTY) return false;
            r += rowDir;
            c += colDir;
        }
        
        return true;
    }
    
    PieceType getPieceTypeFromChar(char c) const {
        switch(toupper(c)) {
            case 'N': return KNIGHT;
            case 'B': return BISHOP;
            case 'R': return ROOK;
            case 'Q': return QUEEN;
            case 'K': return KING;
            default: return EMPTY;
        }
    }
    
    bool findPieceMove(PieceType pieceType, int toRow, int toCol, 
                       int& fromRow, int& fromCol, 
                       char fileHint = '\0', char rankHint = '\0') const {
        vector<pair<int,int>> candidates;
        
        // Find all pieces of the right type and color
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].type == pieceType && board[r][c].color == currentTurn) {
                    // Apply hints if provided
                    if (fileHint != '\0' && c != (fileHint - 'a')) continue;
                    if (rankHint != '\0' && r != (8 - (rankHint - '0'))) continue;
                    
                    if (isValidMove(r, c, toRow, toCol)) {
                        candidates.push_back({r, c});
                    }
                }
            }
        }
        
        if (candidates.size() == 1) {
            fromRow = candidates[0].first;
            fromCol = candidates[0].second;
            return true;
        }
        
        return false;
    }
    
    bool makeMove(const string& move) {
        if (move.empty()) return false;
        
        string cleanMove = move;
        
        // Remove check/checkmate symbols
        while (!cleanMove.empty() && (cleanMove.back() == '+' || cleanMove.back() == '#' || cleanMove.back() == '!')) {
            cleanMove.pop_back();
        }
        
        if (cleanMove.empty()) return false;
        
        // Handle castling
        if (cleanMove == "O-O" || cleanMove == "0-0" || cleanMove == "o-o") {
            // Kingside castling
            int row = (currentTurn == WHITE) ? 7 : 0;
            return makeMove(row == 7 ? "e1g1" : "e8g8");
        }
        if (cleanMove == "O-O-O" || cleanMove == "0-0-0" || cleanMove == "o-o-o") {
            // Queenside castling
            int row = (currentTurn == WHITE) ? 7 : 0;
            return makeMove(row == 7 ? "e1c1" : "e8c8");
        }
        
        // Try long notation first: e2e4 or e2-e4 or e2xe4
        if (cleanMove.length() >= 4) {
            // Check if it looks like long notation (starts with a file and rank)
            if (cleanMove[0] >= 'a' && cleanMove[0] <= 'h' &&
                cleanMove[1] >= '1' && cleanMove[1] <= '8') {
                
                string from = cleanMove.substr(0, 2);
                string to;
                
                size_t toStart = 2;
                if (cleanMove.length() > 2 && (cleanMove[2] == '-' || cleanMove[2] == 'x' || cleanMove[2] == 'X')) {
                    toStart = 3;
                }
                
                if (cleanMove.length() >= toStart + 2) {
                    to = cleanMove.substr(toStart, 2);
                    
                    int fromRow, fromCol, toRow, toCol;
                    if (parseSquare(from, fromRow, fromCol) && parseSquare(to, toRow, toCol)) {
                        if (isValidMove(fromRow, fromCol, toRow, toCol)) {
                            // Check if this is castling
                            bool isCastling = (board[fromRow][fromCol].type == KING && abs(toCol - fromCol) == 2);
                            
                            // Check if this is an en passant capture
                            bool isEnPassant = false;
                            if (board[fromRow][fromCol].type == PAWN && toCol != fromCol && board[toRow][toCol].type == EMPTY) {
                                isEnPassant = true;
                            }
                            
                            // Save move to history
                            int prevEnPassant = enPassantCol;
                            Piece capturedPiece = isEnPassant ? 
                                board[(board[fromRow][fromCol].color == WHITE) ? toRow + 1 : toRow - 1][toCol] :
                                board[toRow][toCol];
                            
                            // Update half-move clock
                            bool isPawnMove = (board[fromRow][fromCol].type == PAWN);
                            bool isCapture = (capturedPiece.type != EMPTY);
                            int prevHalfMove = halfMoveClock;
                            
                            if (isPawnMove || isCapture) {
                                halfMoveClock = 0;
                            } else {
                                halfMoveClock++;
                            }
                            
                            moveHistory.push_back(Move(fromRow, fromCol, toRow, toCol, capturedPiece, isEnPassant, prevEnPassant, isCastling, 0.0, "", prevHalfMove));
                            
                            // Make the move
                            board[toRow][toCol] = board[fromRow][fromCol];
                            board[fromRow][fromCol] = Piece();
                            
                            // Handle pawn promotion
                            if (board[toRow][toCol].type == PAWN) {
                                if ((board[toRow][toCol].color == WHITE && toRow == 0) ||
                                    (board[toRow][toCol].color == BLACK && toRow == 7)) {
                                    // Check if promotion piece is specified in move string
                                    PieceType promoteTo = QUEEN;  // Default to queen
                                    if (cleanMove.length() > toStart + 2) {
                                        char promoChar = tolower(cleanMove[toStart + 2]);
                                        switch(promoChar) {
                                            case 'q': promoteTo = QUEEN; break;
                                            case 'r': promoteTo = ROOK; break;
                                            case 'b': promoteTo = BISHOP; break;
                                            case 'n': promoteTo = KNIGHT; break;
                                            default: promoteTo = QUEEN; break;
                                        }
                                    }
                                    board[toRow][toCol].type = promoteTo;
                                }
                            }
                            
                            // Handle castling - move the rook
                            if (isCastling) {
                                if (toCol == 6) {  // Kingside
                                    board[toRow][5] = board[toRow][7];
                                    board[toRow][7] = Piece();
                                } else if (toCol == 2) {  // Queenside
                                    board[toRow][3] = board[toRow][0];
                                    board[toRow][0] = Piece();
                                }
                            }
                            
                            // Handle en passant capture (remove the captured pawn)
                            if (isEnPassant) {
                                int capturedPawnRow = (board[toRow][toCol].color == WHITE) ? toRow + 1 : toRow - 1;
                                board[capturedPawnRow][toCol] = Piece();
                            }
                            
                            // Update castling rights
                            if (board[toRow][toCol].type == KING) {
                                if (board[toRow][toCol].color == WHITE) {
                                    whiteKingMoved = true;
                                } else {
                                    blackKingMoved = true;
                                }
                            }
                            if (board[toRow][toCol].type == ROOK) {
                                if (board[toRow][toCol].color == WHITE) {
                                    if (fromCol == 7) whiteRookKingsideMoved = true;
                                    if (fromCol == 0) whiteRookQueensideMoved = true;
                                } else {
                                    if (fromCol == 7) blackRookKingsideMoved = true;
                                    if (fromCol == 0) blackRookQueensideMoved = true;
                                }
                            }
                            
                            // Update en passant availability
                            enPassantCol = -1;  // Reset
                            if (board[toRow][toCol].type == PAWN && abs(toRow - fromRow) == 2) {
                                // Pawn moved two squares, enable en passant for next move
                                enPassantCol = toCol;
                            }
                            
                            currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
                            
                            // Track position for repetition detection
                            string posHash = PositionHash::hashPosition(*this);
                            positionHistory[posHash]++;
                            
                            return true;
                        }
                    }
                }
            }
        }
        
        // Parse standard algebraic notation (e.g., e4, Nf3, Bxc4, Nbd7, R1a3)
        size_t idx = 0;
        PieceType pieceType = PAWN;
        char fileHint = '\0';
        char rankHint = '\0';
        bool isCapture = false;
        
        // Check if first character is a piece (uppercase letter)
        if (idx < cleanMove.length() && isupper(cleanMove[idx])) {
            pieceType = getPieceTypeFromChar(cleanMove[idx]);
            if (pieceType == EMPTY) return false;
            idx++;
        }
        
        // Now we need to find: [fileHint][rankHint][x]destination
        // Examples: e4, exd5, Nf3, Nbd7, R1a3, Bxc4
        
        // Look for file hint (for disambiguation or pawn capture)
        if (idx < cleanMove.length() && cleanMove[idx] >= 'a' && cleanMove[idx] <= 'h') {
            // This could be:
            // 1. A pawn's destination file (e4)
            // 2. A pawn's source file for capture (exd5)
            // 3. A piece's file disambiguation (Nbd7)
            
            // Peek ahead to determine which
            if (idx + 1 < cleanMove.length()) {
                char next = cleanMove[idx + 1];
                if (next == 'x' || next == 'X') {
                    // It's a capture - this is the source file
                    fileHint = cleanMove[idx];
                    idx++;
                } else if (next >= '1' && next <= '8') {
                    // Next is a rank
                    if (idx + 2 >= cleanMove.length()) {
                        // This is the destination (e.g., "e4")
                        // Don't increment idx, let it be parsed as destination below
                    } else {
                        // There's more after - could be disambiguation or destination
                        // If the piece is a pawn and this looks like "e4", treat as destination
                        // Otherwise treat as disambiguation
                        if (pieceType != PAWN) {
                            fileHint = cleanMove[idx];
                            idx++;
                        }
                        // For pawns, leave it as the destination
                    }
                } else if (next >= 'a' && next <= 'h') {
                    // Next is also a file - this must be disambiguation
                    fileHint = cleanMove[idx];
                    idx++;
                }
            }
        }
        
        // Look for rank hint
        if (idx < cleanMove.length() && cleanMove[idx] >= '1' && cleanMove[idx] <= '8') {
            // This could be part of destination or a rank hint
            if (idx + 1 < cleanMove.length()) {
                char next = cleanMove[idx + 1];
                if (next == 'x' || next == 'X' || (next >= 'a' && next <= 'h')) {
                    // It's a rank hint
                    rankHint = cleanMove[idx];
                    idx++;
                }
            }
        }
        
        // Check for capture symbol
        if (idx < cleanMove.length() && (cleanMove[idx] == 'x' || cleanMove[idx] == 'X')) {
            isCapture = true;
            idx++;
        }
        
        // Get destination square (must be at least 2 characters: file + rank)
        if (idx + 1 >= cleanMove.length()) return false;
        
        string destSquare = cleanMove.substr(idx, 2);
        int toRow, toCol;
        if (!parseSquare(destSquare, toRow, toCol)) return false;
        
        // For pawn moves without file hint, use the destination file as hint
        if (pieceType == PAWN && fileHint == '\0' && !isCapture) {
            fileHint = destSquare[0];
        }
        
        // Find the piece that can make this move
        int fromRow, fromCol;
        if (findPieceMove(pieceType, toRow, toCol, fromRow, fromCol, fileHint, rankHint)) {
            // Check if this is castling
            bool isCastling = (board[fromRow][fromCol].type == KING && abs(toCol - fromCol) == 2);
            
            // Check if this is an en passant capture
            bool isEnPassant = false;
            if (board[fromRow][fromCol].type == PAWN && toCol != fromCol && board[toRow][toCol].type == EMPTY) {
                isEnPassant = true;
            }
            
            // Save move to history
            int prevEnPassant = enPassantCol;
            Piece capturedPiece = isEnPassant ? 
                board[(board[fromRow][fromCol].color == WHITE) ? toRow + 1 : toRow - 1][toCol] :
                board[toRow][toCol];
            
            // Update half-move clock
            bool isPawnMove = (board[fromRow][fromCol].type == PAWN);
            bool isCapture = (capturedPiece.type != EMPTY);
            int prevHalfMove = halfMoveClock;
            
            if (isPawnMove || isCapture) {
                halfMoveClock = 0;
            } else {
                halfMoveClock++;
            }
            
            moveHistory.push_back(Move(fromRow, fromCol, toRow, toCol, capturedPiece, isEnPassant, prevEnPassant, isCastling, 0.0, "", prevHalfMove));
            
            // Make the move
            board[toRow][toCol] = board[fromRow][fromCol];
            board[fromRow][fromCol] = Piece();
            
            // Handle castling - move the rook
            if (isCastling) {
                if (toCol == 6) {  // Kingside
                    board[toRow][5] = board[toRow][7];
                    board[toRow][7] = Piece();
                } else if (toCol == 2) {  // Queenside
                    board[toRow][3] = board[toRow][0];
                    board[toRow][0] = Piece();
                }
            }
            
            // Handle en passant capture (remove the captured pawn)
            if (isEnPassant) {
                int capturedPawnRow = (board[toRow][toCol].color == WHITE) ? toRow + 1 : toRow - 1;
                board[capturedPawnRow][toCol] = Piece();
            }
            
            // Update castling rights
            if (board[toRow][toCol].type == KING) {
                if (board[toRow][toCol].color == WHITE) {
                    whiteKingMoved = true;
                } else {
                    blackKingMoved = true;
                }
            }
            if (board[toRow][toCol].type == ROOK) {
                if (board[toRow][toCol].color == WHITE) {
                    if (fromCol == 7) whiteRookKingsideMoved = true;
                    if (fromCol == 0) whiteRookQueensideMoved = true;
                } else {
                    if (fromCol == 7) blackRookKingsideMoved = true;
                    if (fromCol == 0) blackRookQueensideMoved = true;
                }
            }
            
            // Update en passant availability
            enPassantCol = -1;  // Reset
            if (board[toRow][toCol].type == PAWN && abs(toRow - fromRow) == 2) {
                // Pawn moved two squares, enable en passant for next move
                enPassantCol = toCol;
            }
            
            // Check for pawn promotion
            if (board[toRow][toCol].type == PAWN) {
                if ((board[toRow][toCol].color == WHITE && toRow == 0) ||
                    (board[toRow][toCol].color == BLACK && toRow == 7)) {
                    // Pawn reached the end - promote it
                    PieceType promoteTo = QUEEN; // Default to queen
                    
                    // Check if there's a promotion piece specified (e.g., e8=Q, e8Q, e1=N)
                    size_t promotePos = cleanMove.find('=');
                    if (promotePos == string::npos) {
                        // Try without '=' sign
                        if (idx + 2 < cleanMove.length()) {
                            char promoPiece = toupper(cleanMove[cleanMove.length() - 1]);
                            PieceType promoType = getPieceTypeFromChar(promoPiece);
                            if (promoType != EMPTY && promoType != PAWN && promoType != KING) {
                                promoteTo = promoType;
                            }
                        }
                    } else if (promotePos + 1 < cleanMove.length()) {
                        char promoPiece = toupper(cleanMove[promotePos + 1]);
                        PieceType promoType = getPieceTypeFromChar(promoPiece);
                        if (promoType != EMPTY && promoType != PAWN && promoType != KING) {
                            promoteTo = promoType;
                        }
                    }
                    
                    board[toRow][toCol].type = promoteTo;
                }
            }
            
            currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
            
            // Track position for repetition detection
            string posHash = PositionHash::hashPosition(*this);
            positionHistory[posHash]++;
            
            return true;
        }
        
        return false;
    }
    
    Color getCurrentTurn() const {
        return currentTurn;
    }
    
    // AI-related methods
    vector<ChessMove> generateLegalMoves(Color color) const;
    double evaluatePosition(Color perspective) const;  // material_heuristic
    
    // Apply and undo moves (for AI search)
    bool applyMove(const ChessMove& move);
    bool applyMoveWithSAN(const ChessMove& move, const string& san, double eval);
    void undoLastMove();
    
    // Undo for human (undoes both player and AI move)
    bool undoHumanMove();
    
    // Get piece at position (for AI evaluation)
    Piece getPieceAt(int row, int col) const {
        if (row < 0 || row > 7 || col < 0 || col > 7) return Piece();
        return board[row][col];
    }
    
    // Set evaluation for last move (for PGN export) - white perspective
    void setLastMoveEvaluation(double eval, Color movingColor) {
        if (!moveHistory.empty()) {
            // Store evaluation from white's perspective
            moveHistory.back().evaluation = (movingColor == WHITE) ? eval : -eval;
        }
    }
    
    // Set SAN notation for last move
    void setLastMoveSAN(const string& san) {
        if (!moveHistory.empty()) {
            moveHistory.back().sanNotation = san;
        }
    }
    
    // PGN export
    string exportPGN(const string& whitePlayer, const string& blackPlayer, const string& result) const;
};

// Implementation of ChessMove::toAlgebraic()
string ChessMove::toAlgebraic() const {
    if (!isValid()) return "invalid";
    
    string from = string(1, 'a' + fromCol) + to_string(8 - fromRow);
    string to = string(1, 'a' + toCol) + to_string(8 - toRow);
    string result = from + to;
    
    if (promotionPiece != EMPTY) {
        char promoChar = 'q';
        switch(promotionPiece) {
            case QUEEN: promoChar = 'q'; break;
            case ROOK: promoChar = 'r'; break;
            case BISHOP: promoChar = 'b'; break;
            case KNIGHT: promoChar = 'n'; break;
            default: break;
        }
        result += promoChar;
    }
    
    return result;
}

// Implementation of ChessMove::toSAN() - Standard Algebraic Notation
string ChessMove::toSAN(const ChessBoard& board) const {
    if (!isValid()) return "invalid";
    
    Piece piece = board.getPieceAt(fromRow, fromCol);
    Piece target = board.getPieceAt(toRow, toCol);
    
    string result = "";
    
    // Check for castling
    if (piece.type == KING && abs(toCol - fromCol) == 2) {
        return (toCol > fromCol) ? "O-O" : "O-O-O";
    }
    
    // Piece letter (except for pawns)
    if (piece.type != PAWN) {
        switch(piece.type) {
            case KNIGHT: result += "N"; break;
            case BISHOP: result += "B"; break;
            case ROOK: result += "R"; break;
            case QUEEN: result += "Q"; break;
            case KING: result += "K"; break;
            default: break;
        }
        
        // Check if we need disambiguation
        vector<ChessMove> legalMoves = board.generateLegalMoves(piece.color);
        bool needFile = false, needRank = false;
        
        for (const ChessMove& m : legalMoves) {
            if (m.toRow == toRow && m.toCol == toCol && 
                (m.fromRow != fromRow || m.fromCol != fromCol)) {
                Piece otherPiece = board.getPieceAt(m.fromRow, m.fromCol);
                if (otherPiece.type == piece.type) {
                    if (m.fromCol != fromCol) {
                        needFile = true;
                    } else {
                        needRank = true;
                    }
                }
            }
        }
        
        if (needFile) result += string(1, 'a' + fromCol);
        if (needRank) result += to_string(8 - fromRow);
    } else {
        // Pawn moves: only show file if capturing
        if (fromCol != toCol || target.type != EMPTY) {
            result += string(1, 'a' + fromCol);
        }
    }
    
    // Capture notation
    if (target.type != EMPTY || (piece.type == PAWN && fromCol != toCol)) {
        result += "x";
    }
    
    // Destination square
    result += string(1, 'a' + toCol);
    result += to_string(8 - toRow);
    
    // Promotion
    if (promotionPiece != EMPTY) {
        result += "=";
        switch(promotionPiece) {
            case QUEEN: result += "Q"; break;
            case ROOK: result += "R"; break;
            case BISHOP: result += "B"; break;
            case KNIGHT: result += "N"; break;
            default: break;
        }
    }
    
    // Note: Check/checkmate symbols would be added by caller after move is made
    return result;
}

// Position hashing for repetition detection
string PositionHash::hashPosition(const ChessBoard& board) {
    // Simple string-based hash
    stringstream ss;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board.getPieceAt(row, col);
            if (p.type != EMPTY) {
                ss << (char)('0' + p.type) << (char)('0' + p.color) << row << col << "|";
            }
        }
    }
    
    // Include turn
    ss << (board.getCurrentTurn() == WHITE ? "W" : "B");
    
    return ss.str();
}

// Generate all legal moves for a given color
vector<ChessMove> ChessBoard::generateLegalMoves(Color color) const {
    vector<ChessMove> legalMoves;
    
    for (int fromRow = 0; fromRow < 8; fromRow++) {
        for (int fromCol = 0; fromCol < 8; fromCol++) {
            if (board[fromRow][fromCol].color != color) continue;
            if (board[fromRow][fromCol].type == EMPTY) continue;
            
            // Try all possible destination squares
            for (int toRow = 0; toRow < 8; toRow++) {
                for (int toCol = 0; toCol < 8; toCol++) {
                    if (isValidMove(fromRow, fromCol, toRow, toCol)) {
                        // Check for pawn promotion
                        if (board[fromRow][fromCol].type == PAWN) {
                            if ((color == WHITE && toRow == 0) || (color == BLACK && toRow == 7)) {
                                // Add all promotion options
                                legalMoves.push_back(ChessMove(fromRow, fromCol, toRow, toCol, QUEEN));
                                legalMoves.push_back(ChessMove(fromRow, fromCol, toRow, toCol, ROOK));
                                legalMoves.push_back(ChessMove(fromRow, fromCol, toRow, toCol, BISHOP));
                                legalMoves.push_back(ChessMove(fromRow, fromCol, toRow, toCol, KNIGHT));
                                continue;
                            }
                        }
                        legalMoves.push_back(ChessMove(fromRow, fromCol, toRow, toCol));
                    }
                }
            }
        }
    }
    
    return legalMoves;
}

// Material-based heuristic evaluation
double ChessBoard::evaluatePosition(Color perspective) const {
    // Check for checkmate/stalemate first
    if (isCheckmate(perspective)) {
        return -numeric_limits<double>::infinity();
    }
    
    Color opponent = (perspective == WHITE) ? BLACK : WHITE;
    if (isCheckmate(opponent)) {
        return numeric_limits<double>::infinity();
    }
    
    double score = 0.0;
    
    // Material values
    const double PAWN_VALUE = 1.0;
    const double KNIGHT_VALUE = 3.0;
    const double BISHOP_VALUE = 3.0;
    const double ROOK_VALUE = 5.0;
    const double QUEEN_VALUE = 9.0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board[row][col];
            if (piece.type == EMPTY) continue;
            
            double pieceValue = 0.0;
            switch(piece.type) {
                case PAWN: pieceValue = PAWN_VALUE; break;
                case KNIGHT: pieceValue = KNIGHT_VALUE; break;
                case BISHOP: pieceValue = BISHOP_VALUE; break;
                case ROOK: pieceValue = ROOK_VALUE; break;
                case QUEEN: pieceValue = QUEEN_VALUE; break;
                case KING: pieceValue = 0.0; break;  // King has no material value
                default: pieceValue = 0.0;
            }
            
            if (piece.color == perspective) {
                score += pieceValue;
            } else {
                score -= pieceValue;
            }
        }
    }
    
    return score;
}

// Apply a move (for AI search)
bool ChessBoard::applyMove(const ChessMove& move) {
    if (!move.isValid()) return false;
    
    // Convert ChessMove to string and use existing makeMove
    string moveStr = move.toAlgebraic();
    return makeMove(moveStr);
}

// Apply a move with SAN notation and evaluation (for game moves)
bool ChessBoard::applyMoveWithSAN(const ChessMove& move, const string& san, double eval) {
    if (!applyMove(move)) return false;
    
    // Update the last move with SAN and evaluation
    if (!moveHistory.empty()) {
        moveHistory.back().sanNotation = san;
        moveHistory.back().evaluation = eval;
    }
    return true;
}

// Undo last move (already implemented as undoMove)
void ChessBoard::undoLastMove() {
    undoMove();
}

// Undo for human players (undoes both human and AI move)
bool ChessBoard::undoHumanMove() {
    if (moveHistory.size() < 2) {
        // Not enough moves to undo both
        if (moveHistory.size() == 1) {
            return undoMove();  // Just undo the one move
        }
        return false;
    }
    
    // Undo AI move first, then human move
    undoMove();
    undoMove();
    return true;
}

// Export game to PGN format with evaluation tags
string ChessBoard::exportPGN(const string& whitePlayer, const string& blackPlayer, const string& result) const {
    stringstream pgn;
    
    // PGN headers
    time_t now = time(0);
    tm* ltm = localtime(&now);
    
    pgn << "[Event \"Casual Game\"]\n";
    pgn << "[Site \"CLI Chess\"]\n";
    pgn << "[Date \"" << (1900 + ltm->tm_year) << "."
        << setfill('0') << setw(2) << (1 + ltm->tm_mon) << "."
        << setfill('0') << setw(2) << ltm->tm_mday << "\"]\n";
    pgn << "[Round \"1\"]\n";
    pgn << "[White \"" << whitePlayer << "\"]\n";
    pgn << "[Black \"" << blackPlayer << "\"]\n";
    pgn << "[Result \"" << result << "\"]\n\n";
    
    // Move history with SAN notation and evaluation tags
    int moveNum = 1;
    for (size_t i = 0; i < moveHistory.size(); i++) {
        const Move& m = moveHistory[i];
        
        if (i % 2 == 0) {
            pgn << moveNum << ". ";
        }
        
        // Use SAN if available, otherwise fall back to algebraic
        if (!m.sanNotation.empty()) {
            pgn << m.sanNotation;
        } else {
            pgn << string(1, 'a' + m.fromCol) << (8 - m.fromRow)
                << string(1, 'a' + m.toCol) << (8 - m.toRow);
        }
        
        // Add evaluation tag if available (white perspective: + = white winning)
        if (m.evaluation != 0.0) {
            pgn << " {[%eval ";
            if (m.evaluation >= 100000.0) {
                int mateDist = (int)((m.evaluation - 100000.0) / 100.0) + 1;
                pgn << "#" << mateDist;  // Mate in N for white
            } else if (m.evaluation <= -100000.0) {
                int mateDist = (int)((-m.evaluation - 100000.0) / 100.0) + 1;
                pgn << "#-" << mateDist;  // Mate in N for black
            } else {
                pgn << fixed << setprecision(2) << m.evaluation;
            }
            pgn << "]}";
        }
        
        if (i % 2 == 1) {
            pgn << " ";
            moveNum++;
        } else {
            pgn << " ";
        }
    }
    
    pgn << result << "\n";
    
    return pgn.str();
}

// ============================================================================
// AI IMPLEMENTATIONS
// ============================================================================

// Random AI - picks a random legal move
class RandomAI : public ChessAI {
public:
    string getName() const override {
        return "Random AI";
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) {
            return ChessMove();  // No legal moves
        }
        
        int randomIndex = rand() % legalMoves.size();
        return legalMoves[randomIndex];
    }
};

// Move ordering helper - orders moves for better alpha-beta pruning
class MoveOrderer {
public:
    static int scoreMoveForOrdering(const ChessMove& move, const ChessBoard& board) {
        int score = 0;
        
        Piece fromPiece = board.getPieceAt(move.fromRow, move.fromCol);
        Piece toPiece = board.getPieceAt(move.toRow, move.toCol);
        
        // MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        if (toPiece.type != EMPTY) {
            int victimValue = getPieceValue(toPiece.type);
            int attackerValue = getPieceValue(fromPiece.type);
            score += 1000 + (victimValue * 10 - attackerValue);
        }
        
        // Promotions are very good
        if (move.promotionPiece == QUEEN) {
            score += 900;
        } else if (move.promotionPiece != EMPTY) {
            score += 800;
        }
        
        // Center control bonus
        if ((move.toRow == 3 || move.toRow == 4) && (move.toCol == 3 || move.toCol == 4)) {
            score += 50;
        }
        
        return score;
    }
    
    static void orderMoves(vector<ChessMove>& moves, const ChessBoard& board) {
        vector<pair<int, ChessMove>> scoredMoves;
        for (const ChessMove& move : moves) {
            int score = scoreMoveForOrdering(move, board);
            scoredMoves.push_back({score, move});
        }
        
        // Sort by score (highest first)
        sort(scoredMoves.begin(), scoredMoves.end(), 
             [](const pair<int, ChessMove>& a, const pair<int, ChessMove>& b) {
                 return a.first > b.first;
             });
        
        // Extract sorted moves
        moves.clear();
        for (const auto& sm : scoredMoves) {
            moves.push_back(sm.second);
        }
    }
    
private:
    static int getPieceValue(PieceType type) {
        switch(type) {
            case PAWN: return 1;
            case KNIGHT: return 3;
            case BISHOP: return 3;
            case ROOK: return 5;
            case QUEEN: return 9;
            case KING: return 100;
            default: return 0;
        }
    }
};

// Store evaluation globally for the last AI move (for PGN export)
double lastAIEvaluation = 0.0;

// Materialistic AI - uses only material evaluation with alpha-beta pruning
class MaterialisticAI : public ChessAI {
private:
    int maxDepth;
    
    double materialEval(const ChessBoard& board, Color perspective) const {
        // Check for checkmate first
        if (board.isCheckmate(perspective)) {
            return -numeric_limits<double>::infinity();
        }
        Color opponent = (perspective == WHITE) ? BLACK : WHITE;
        if (board.isCheckmate(opponent)) {
            return numeric_limits<double>::infinity();
        }
        
        // Pure material count
        return board.evaluatePosition(perspective);
    }
    
    double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, Color maximizingColor, int plyFromRoot) {
        // Terminal conditions
        if (depth == 0) {
            return materialEval(board, maximizingColor);
        }
        
        Color currentColor = board.getCurrentTurn();
        vector<ChessMove> legalMoves = board.generateLegalMoves(currentColor);
        
        if (legalMoves.empty()) {
            if (board.isInCheck(currentColor)) {
                // Checkmate
                // When WE are getting mated: return large negative, prefer slower (more plies = less negative)
                // When OPPONENT is getting mated: return large positive, prefer faster (fewer plies = more positive)
                if (currentColor == maximizingColor) {
                    // We're getting mated - bad! Prefer delaying it (more plies = less bad)
                    return -100000.0 + plyFromRoot * 100.0;
                } else {
                    // Opponent is getting mated - good! Prefer faster mate (fewer plies = more good)
                    return 100000.0 - plyFromRoot * 100.0;
                }
            } else {
                // Stalemate
                return 0.0;
            }
        }
        
        // Move ordering for better pruning
        MoveOrderer::orderMoves(legalMoves, board);
        
        if (currentColor == maximizingColor) {
            double maxEval = -numeric_limits<double>::infinity();
            for (const ChessMove& move : legalMoves) {
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) {
                    break;
                }
            }
            return maxEval;
        } else {
            double minEval = numeric_limits<double>::infinity();
            for (const ChessMove& move : legalMoves) {
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) {
                    break;
                }
            }
            return minEval;
        }
    }
    
public:
    MaterialisticAI(int depth = 3) : maxDepth(depth) {}
    
    string getName() const override {
        return "Materialistic AI (depth " + to_string(maxDepth) + ")";
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) {
            return ChessMove();
        }
        
        // Move ordering
        MoveOrderer::orderMoves(legalMoves, board);
        
        ChessMove bestMove = legalMoves[0];  // Always have fallback
        double bestEval = -100000.0;
        double alpha = -numeric_limits<double>::infinity();
        double beta = numeric_limits<double>::infinity();
        
        ChessBoard searchBoard = board;
        
        for (const ChessMove& move : legalMoves) {
            searchBoard.applyMove(move);
            double eval = alphaBeta(searchBoard, maxDepth - 1, alpha, beta, color, 1);
            searchBoard.undoLastMove();
            
            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
            }
            alpha = max(alpha, eval);
        }
        
        // Convert to objective (white perspective) for output
        double objectiveEval = (color == WHITE) ? bestEval : -bestEval;
        
        cout << "AI evaluation: ";
        if (objectiveEval >= 100000.0) {
            int mateDist = (int)((objectiveEval - 100000.0) / 100.0) + 1;
            cout << "#" << mateDist;
        } else if (objectiveEval <= -100000.0) {
            int mateDist = (int)((-objectiveEval - 100000.0) / 100.0) + 1;
            cout << "#-" << mateDist;
        } else {
            cout << fixed << setprecision(2) << objectiveEval;
        }
        cout << endl;
        
        // Store for PGN export (white perspective)
        lastAIEvaluation = objectiveEval;
        
        return bestMove;
    }
};

// Positional AI - advanced evaluation with positional factors
class PositionalAI_DepthBased : public ChessAI {
private:
    int maxDepth;
    
    double positionalEval(const ChessBoard& board, Color perspective) const {
        // Piece-square tables (defined inline)
        static const double pawnTable[8][8] = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0},
            {1.0, 1.0, 2.0, 3.0, 3.0, 2.0, 1.0, 1.0},
            {0.5, 0.5, 1.0, 2.5, 2.5, 1.0, 0.5, 0.5},
            {0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 0.0},
            {0.5,-0.5,-1.0, 0.0, 0.0,-1.0,-0.5, 0.5},
            {0.5, 1.0, 1.0,-2.0,-2.0, 1.0, 1.0, 0.5},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
        };
        
        static const double knightTable[8][8] = {
            {-5.0,-4.0,-3.0,-3.0,-3.0,-3.0,-4.0,-5.0},
            {-4.0,-2.0, 0.0, 0.0, 0.0, 0.0,-2.0,-4.0},
            {-3.0, 0.0, 1.0, 1.5, 1.5, 1.0, 0.0,-3.0},
            {-3.0, 0.5, 1.5, 2.0, 2.0, 1.5, 0.5,-3.0},
            {-3.0, 0.0, 1.5, 2.0, 2.0, 1.5, 0.0,-3.0},
            {-3.0, 0.5, 1.0, 1.5, 1.5, 1.0, 0.5,-3.0},
            {-4.0,-2.0, 0.0, 0.5, 0.5, 0.0,-2.0,-4.0},
            {-5.0,-4.0,-3.0,-3.0,-3.0,-3.0,-4.0,-5.0}
        };
        
        static const double bishopTable[8][8] = {
            {-2.0,-1.0,-1.0,-1.0,-1.0,-1.0,-1.0,-2.0},
            {-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,-1.0},
            {-1.0, 0.0, 0.5, 1.0, 1.0, 0.5, 0.0,-1.0},
            {-1.0, 0.5, 0.5, 1.0, 1.0, 0.5, 0.5,-1.0},
            {-1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0,-1.0},
            {-1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,-1.0},
            {-1.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5,-1.0},
            {-2.0,-1.0,-1.0,-1.0,-1.0,-1.0,-1.0,-2.0}
        };
        
        // Check for checkmate first - but return large negative, not -inf
        // This allows finding the move that delays checkmate longest
        if (board.isCheckmate(perspective)) {
            return -100000.0;  // Very bad but not -inf so we can compare moves
        }
        Color opponent = (perspective == WHITE) ? BLACK : WHITE;
        if (board.isCheckmate(opponent)) {
            return 100000.0;  // Very good but not +inf
        }
        
        double score = 0.0;
        
        // Material values (slightly favor bishops)
        const double PAWN_VALUE = 1.0;
        const double KNIGHT_VALUE = 3.0;
        const double BISHOP_VALUE = 3.2;
        const double ROOK_VALUE = 5.0;
        const double QUEEN_VALUE = 9.0;
        
        // Track pawns for structure evaluation
        int whitePawns[8] = {0}; // pawns per file
        int blackPawns[8] = {0};
        
        // Track piece development (opening principles)
        int whiteDeveloped = 0;
        int blackDeveloped = 0;
        int whiteMinorPieces = 0;
        int blackMinorPieces = 0;
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                Piece piece = board.getPieceAt(row, col);
                if (piece.type == EMPTY) continue;
                
                double pieceValue = 0.0;
                double positionBonus = 0.0;
                
                switch(piece.type) {
                    case PAWN: {
                        pieceValue = PAWN_VALUE;
                        positionBonus = (piece.color == WHITE) ? 
                            pawnTable[row][col] / 10.0 : 
                            pawnTable[7-row][col] / 10.0;
                        
                        if (piece.color == WHITE) whitePawns[col]++;
                        else blackPawns[col]++;
                        break;
                    }
                        
                    case KNIGHT: {
                        pieceValue = KNIGHT_VALUE;
                        positionBonus = knightTable[row][col] / 10.0;
                        
                        // Count minor pieces for development
                        if (piece.color == WHITE) {
                            whiteMinorPieces++;
                            if (row != 7) whiteDeveloped++;  // Moved from back rank
                        } else {
                            blackMinorPieces++;
                            if (row != 0) blackDeveloped++;
                        }
                        
                        // KNIGHT SAFETY: Penalize knights with few escape squares
                        // Check how many squares the knight can move to
                        int escapeSquares = 0;
                        int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
                        for (int i = 0; i < 8; i++) {
                            int newRow = row + knightMoves[i][0];
                            int newCol = col + knightMoves[i][1];
                            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                                Piece target = board.getPieceAt(newRow, newCol);
                                if (target.type == EMPTY || target.color != piece.color) {
                                    escapeSquares++;
                                }
                            }
                        }
                        
                        // Heavily penalize knights with ≤2 escape squares (likely trapped)
                        if (escapeSquares <= 2) {
                            positionBonus -= 2.0;  // Major penalty for trapped knights
                        } else if (escapeSquares <= 3) {
                            positionBonus -= 0.5;  // Minor penalty for restricted knights
                        }
                        
                        // OPENING PRINCIPLE: Don't move knights to e4/e5/d4/d5 too early
                        // unless well-supported
                        bool isEarlyGame2 = (whiteDeveloped + blackDeveloped < 6);
                        if (isEarlyGame2 && ((row == 4 && (col == 3 || col == 4)) || 
                                            (row == 3 && (col == 3 || col == 4)))) {
                            // Check if knight is supported by pawns
                            bool supported = false;
                            if (piece.color == WHITE) {
                                // White knight on central square
                                if (row < 7) {
                                    if (col > 0 && board.getPieceAt(row+1, col-1).type == PAWN && 
                                        board.getPieceAt(row+1, col-1).color == WHITE) supported = true;
                                    if (col < 7 && board.getPieceAt(row+1, col+1).type == PAWN && 
                                        board.getPieceAt(row+1, col+1).color == WHITE) supported = true;
                                }
                            } else {
                                // Black knight on central square
                                if (row > 0) {
                                    if (col > 0 && board.getPieceAt(row-1, col-1).type == PAWN && 
                                        board.getPieceAt(row-1, col-1).color == BLACK) supported = true;
                                    if (col < 7 && board.getPieceAt(row-1, col+1).type == PAWN && 
                                        board.getPieceAt(row-1, col+1).color == BLACK) supported = true;
                                }
                            }
                            
                            if (!supported) {
                                positionBonus -= 1.5;  // Penalty for unsupported central knights in opening
                            }
                        }
                        break;
                    }
                        
                    case BISHOP: {
                        pieceValue = BISHOP_VALUE;
                        positionBonus = bishopTable[row][col] / 10.0;
                        
                        if (piece.color == WHITE) {
                            whiteMinorPieces++;
                            if (row != 7) whiteDeveloped++;
                        } else {
                            blackMinorPieces++;
                            if (row != 0) blackDeveloped++;
                        }
                        break;
                    }
                        
                    case ROOK: {
                        pieceValue = ROOK_VALUE;
                        // Bonus for rook on open file
                        if (whitePawns[col] == 0 && blackPawns[col] == 0) {
                            positionBonus += 0.5;
                        } else if ((piece.color == WHITE && whitePawns[col] == 0) ||
                                   (piece.color == BLACK && blackPawns[col] == 0)) {
                            positionBonus += 0.25;  // Semi-open file
                        }
                        break;
                    }
                        
                    case QUEEN: {
                        pieceValue = QUEEN_VALUE;
                        
                        // OPENING PRINCIPLE: Penalize early queen development
                        bool earlyGame2 = (whiteDeveloped + blackDeveloped < 4);
                        if (earlyGame2) {
                            if (piece.color == WHITE && row < 6) {
                                positionBonus -= 1.0;  // Queen moved too early
                            } else if (piece.color == BLACK && row > 1) {
                                positionBonus -= 1.0;
                            }
                        }
                        break;
                    }
                        
                    case KING: {
                        pieceValue = 0.0;
                        // King safety - penalize exposed king in middlegame
                        if (piece.color == WHITE && row == 7) {
                            if (col > 0 && board.getPieceAt(6, col-1).type == PAWN) positionBonus += 0.1;
                            if (board.getPieceAt(6, col).type == PAWN) positionBonus += 0.1;
                            if (col < 7 && board.getPieceAt(6, col+1).type == PAWN) positionBonus += 0.1;
                        } else if (piece.color == BLACK && row == 0) {
                            if (col > 0 && board.getPieceAt(1, col-1).type == PAWN) positionBonus += 0.1;
                            if (board.getPieceAt(1, col).type == PAWN) positionBonus += 0.1;
                            if (col < 7 && board.getPieceAt(1, col+1).type == PAWN) positionBonus += 0.1;
                        }
                        break;
                    }
                        
                    default:
                        pieceValue = 0.0;
                }
                
                double totalValue = pieceValue + positionBonus;
                
                if (piece.color == perspective) {
                    score += totalValue;
                } else {
                    score -= totalValue;
                }
            }
        }
        
        // OPENING PRINCIPLE: Bonus for developing minor pieces
        if (perspective == WHITE) {
            score += whiteDeveloped * 0.15;  // Small bonus per developed piece
            score -= blackDeveloped * 0.15;
        } else {
            score += blackDeveloped * 0.15;
            score -= whiteDeveloped * 0.15;
        }
        
        // Pawn structure evaluation
        for (int col = 0; col < 8; col++) {
            // Doubled pawns penalty
            if (whitePawns[col] > 1) {
                score -= 0.5 * (whitePawns[col] - 1) * (perspective == WHITE ? 1 : -1);
            }
            if (blackPawns[col] > 1) {
                score -= 0.5 * (blackPawns[col] - 1) * (perspective == BLACK ? 1 : -1);
            }
            
            // Isolated pawns penalty
            bool whiteIsolated = (whitePawns[col] > 0 && 
                                  (col == 0 || whitePawns[col-1] == 0) &&
                                  (col == 7 || whitePawns[col+1] == 0));
            bool blackIsolated = (blackPawns[col] > 0 && 
                                  (col == 0 || blackPawns[col-1] == 0) &&
                                  (col == 7 || blackPawns[col+1] == 0));
            
            if (whiteIsolated) {
                score -= 0.5 * (perspective == WHITE ? 1 : -1);
            }
            if (blackIsolated) {
                score -= 0.5 * (perspective == BLACK ? 1 : -1);
            }
            
            // Passed pawns bonus (simplified check)
            for (int row = 0; row < 8; row++) {
                Piece p = board.getPieceAt(row, col);
                if (p.type == PAWN) {
                    bool isPassed = true;
                    if (p.color == WHITE) {
                        for (int r = row - 1; r >= 0; r--) {
                            if (board.getPieceAt(r, col).type == PAWN && 
                                board.getPieceAt(r, col).color == BLACK) {
                                isPassed = false;
                                break;
                            }
                            if (col > 0 && board.getPieceAt(r, col-1).type == PAWN && 
                                board.getPieceAt(r, col-1).color == BLACK) {
                                isPassed = false;
                                break;
                            }
                            if (col < 7 && board.getPieceAt(r, col+1).type == PAWN && 
                                board.getPieceAt(r, col+1).color == BLACK) {
                                isPassed = false;
                                break;
                            }
                        }
                        if (isPassed) {
                            double passedBonus = 0.5 + (6.0 - row) * 0.1;
                            score += passedBonus * (perspective == WHITE ? 1 : -1);
                        }
                    } else {
                        for (int r = row + 1; r < 8; r++) {
                            if (board.getPieceAt(r, col).type == PAWN && 
                                board.getPieceAt(r, col).color == WHITE) {
                                isPassed = false;
                                break;
                            }
                            if (col > 0 && board.getPieceAt(r, col-1).type == PAWN && 
                                board.getPieceAt(r, col-1).color == WHITE) {
                                isPassed = false;
                                break;
                            }
                            if (col < 7 && board.getPieceAt(r, col+1).type == PAWN && 
                                board.getPieceAt(r, col+1).color == WHITE) {
                                isPassed = false;
                                break;
                            }
                        }
                        if (isPassed) {
                            double passedBonus = 0.5 + (row - 1) * 0.1;
                            score += passedBonus * (perspective == BLACK ? 1 : -1);
                        }
                    }
                }
            }
        }
        
        // Mobility bonus
        int myMoves = board.generateLegalMoves(perspective).size();
        int opponentMoves = board.generateLegalMoves(opponent).size();
        score += (myMoves - opponentMoves) * 0.1;
        
        // Center control bonus
        for (int row = 3; row <= 4; row++) {
            for (int col = 3; col <= 4; col++) {
                Piece p = board.getPieceAt(row, col);
                if (p.type != EMPTY) {
                    double centerBonus = 0.3;
                    if (p.color == perspective) {
                        score += centerBonus;
                    } else {
                        score -= centerBonus;
                    }
                }
            }
        }
        
        return score;
    }
    
    // Quiescence search - search captures to avoid horizon effect
    double quiesce(ChessBoard& board, double alpha, double beta, Color maximizingColor, int depth = 3) {
        if (depth == 0) {
            return positionalEval(board, maximizingColor);
        }
        
        double standPat = positionalEval(board, maximizingColor);
        
        Color currentColor = board.getCurrentTurn();
        if (currentColor == maximizingColor) {
            if (standPat >= beta) return beta;
            if (alpha < standPat) alpha = standPat;
        } else {
            if (standPat <= alpha) return alpha;
            if (beta > standPat) beta = standPat;
        }
        
        // Only consider captures and promotions
        vector<ChessMove> allMoves = board.generateLegalMoves(currentColor);
        vector<ChessMove> captureMoves;
        
        for (const ChessMove& move : allMoves) {
            Piece target = board.getPieceAt(move.toRow, move.toCol);
            if (target.type != EMPTY || move.promotionPiece != EMPTY) {
                captureMoves.push_back(move);
            }
        }
        
        if (captureMoves.empty()) {
            return standPat;
        }
        
        MoveOrderer::orderMoves(captureMoves, board);
        
        if (currentColor == maximizingColor) {
            for (const ChessMove& move : captureMoves) {
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }
            return alpha;
        } else {
            for (const ChessMove& move : captureMoves) {
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score <= alpha) return alpha;
                if (score < beta) beta = score;
            }
            return beta;
        }
    }
    
    double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, Color maximizingColor) {
        // Terminal conditions
        if (depth == 0) {
            // Use quiescence search at leaf nodes
            return quiesce(board, alpha, beta, maximizingColor);
        }
        
        Color currentColor = board.getCurrentTurn();
        vector<ChessMove> legalMoves = board.generateLegalMoves(currentColor);
        
        if (legalMoves.empty()) {
            if (board.isInCheck(currentColor)) {
                return (currentColor == maximizingColor) ? 
                    -numeric_limits<double>::infinity() : 
                    numeric_limits<double>::infinity();
            } else {
                return 0.0;
            }
        }
        
        // Move ordering
        MoveOrderer::orderMoves(legalMoves, board);
        
        if (currentColor == maximizingColor) {
            double maxEval = -numeric_limits<double>::infinity();
            for (const ChessMove& move : legalMoves) {
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor);
                board.undoLastMove();
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) {
                    break;
                }
            }
            return maxEval;
        } else {
            double minEval = numeric_limits<double>::infinity();
            for (const ChessMove& move : legalMoves) {
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor);
                board.undoLastMove();
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) {
                    break;
                }
            }
            return minEval;
        }
    }
    
public:
    PositionalAI_DepthBased(int depth = 3) : maxDepth(depth) {}
    
    string getName() const override {
        return "Positional AI (depth " + to_string(maxDepth) + ")";
    }
    
    // Public method to access positional evaluation (for PositionalAI to use)
    double evaluatePosition(const ChessBoard& board, Color perspective) const {
        return positionalEval(board, perspective);
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) {
            return ChessMove();  // No legal moves - but this should be caught before calling
        }
        
        // Move ordering
        MoveOrderer::orderMoves(legalMoves, board);
        
        ChessMove bestMove = legalMoves[0];  // Always have at least one move as fallback
        double bestEval = -100000.0;  // Start very low
        double alpha = -numeric_limits<double>::infinity();
        double beta = numeric_limits<double>::infinity();
        
        ChessBoard searchBoard = board;
        
        // If we're in checkmate, find the move that delays it longest
        bool inCheckmate = board.isCheckmate(color);
        
        for (const ChessMove& move : legalMoves) {
            searchBoard.applyMove(move);
            double eval = alphaBeta(searchBoard, maxDepth - 1, alpha, beta, color);
            searchBoard.undoLastMove();
            
            // Always update best move if this is better OR if we haven't found any valid move yet
            if (eval > bestEval || (inCheckmate && bestMove.fromRow == legalMoves[0].fromRow)) {
                bestEval = eval;
                bestMove = move;
            }
            alpha = max(alpha, eval);
        }
        
        // Convert to objective (white perspective) for output
        double objectiveEval = (color == WHITE) ? bestEval : -bestEval;
        
        cout << "AI evaluation: ";
        if (objectiveEval >= 100000.0) {
            cout << "#" << ((int)((objectiveEval - 100000.0) / 1000.0) + 1);
        } else if (objectiveEval <= -100000.0) {
            cout << "#-" << ((int)((-objectiveEval - 100000.0) / 1000.0) + 1);
        } else {
            cout << fixed << setprecision(2) << objectiveEval;
        }
        cout << endl;
        
        // Store for PGN export (white perspective)
        lastAIEvaluation = objectiveEval;
        
        return bestMove;
    }
};

// Iterative Deepening AI with time limits
class IterativeDeepeningAI : public ChessAI {
private:
    double timeLimit;  // Time limit in seconds
    clock_t searchStartTime;
    bool timeExpired;
    ChessMove bestMoveFound;
    double bestEvalFound;
    
    bool isTimeUp() const {
        double elapsed = double(clock() - searchStartTime) / CLOCKS_PER_SEC;
        return elapsed >= timeLimit;
    }
    
    double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, Color maximizingColor) {
        if (timeExpired || isTimeUp()) {
            timeExpired = true;
            return 0.0;  // Return quickly
        }
        
        // Terminal conditions
        if (depth == 0) {
            return quiesce(board, alpha, beta, maximizingColor);
        }
        
        Color currentColor = board.getCurrentTurn();
        vector<ChessMove> legalMoves = board.generateLegalMoves(currentColor);
        
        if (legalMoves.empty()) {
            if (board.isInCheck(currentColor)) {
                return (currentColor == maximizingColor) ? 
                    -100000.0 : 100000.0;
            } else {
                return 0.0;
            }
        }
        
        // Move ordering
        MoveOrderer::orderMoves(legalMoves, board);
        
        if (currentColor == maximizingColor) {
            double maxEval = -100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor);
                board.undoLastMove();
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        } else {
            double minEval = 100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor);
                board.undoLastMove();
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }
    
    double quiesce(ChessBoard& board, double alpha, double beta, Color maximizingColor, int depth = 3) {
        if (timeExpired || depth == 0) {
            return positionalEval(board, maximizingColor);
        }
        
        double standPat = positionalEval(board, maximizingColor);
        
        Color currentColor = board.getCurrentTurn();
        if (currentColor == maximizingColor) {
            if (standPat >= beta) return beta;
            if (alpha < standPat) alpha = standPat;
        } else {
            if (standPat <= alpha) return alpha;
            if (beta > standPat) beta = standPat;
        }
        
        vector<ChessMove> allMoves = board.generateLegalMoves(currentColor);
        vector<ChessMove> captureMoves;
        
        for (const ChessMove& move : allMoves) {
            Piece target = board.getPieceAt(move.toRow, move.toCol);
            if (target.type != EMPTY || move.promotionPiece != EMPTY) {
                captureMoves.push_back(move);
            }
        }
        
        if (captureMoves.empty()) return standPat;
        
        MoveOrderer::orderMoves(captureMoves, board);
        
        if (currentColor == maximizingColor) {
            for (const ChessMove& move : captureMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }
            return alpha;
        } else {
            for (const ChessMove& move : captureMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score <= alpha) return alpha;
                if (score < beta) beta = score;
            }
            return beta;
        }
    }
    
    double positionalEval(const ChessBoard& board, Color perspective) const {
        // Use same evaluation as PositionalAI
        // For brevity, using simplified version - full version would copy PositionalAI's eval
        if (board.isCheckmate(perspective)) {
            return -100000.0;
        }
        Color opponent = (perspective == WHITE) ? BLACK : WHITE;
        if (board.isCheckmate(opponent)) {
            return 100000.0;
        }
        
        // Just use material for now (you can copy full positional eval from PositionalAI)
        return board.evaluatePosition(perspective);
    }
    
public:
    IterativeDeepeningAI(double timeLimitSeconds = 5.0) : timeLimit(timeLimitSeconds) {}
    
    bool usesTimeLimit() const override { return true; }
    
    void setTimeLimit(double seconds) override {
        timeLimit = seconds;
    }
    
    string getName() const override {
        return "Iterative Deepening AI (" + to_string((int)timeLimit) + "s per move)";
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) {
            return ChessMove();
        }
        
        searchStartTime = clock();
        timeExpired = false;
        bestMoveFound = legalMoves[0];
        bestEvalFound = -100000.0;
        
        ChessBoard searchBoard = board;
        
        // Iterative deepening: search depth 1, then 2, then 3, etc.
        for (int depth = 1; depth <= 20; depth++) {
            if (timeExpired || isTimeUp()) {
                break;
            }
            
            MoveOrderer::orderMoves(legalMoves, searchBoard);
            
            ChessMove depthBestMove = legalMoves[0];
            double depthBestEval = -100000.0;
            double alpha = -numeric_limits<double>::infinity();
            double beta = numeric_limits<double>::infinity();
            
            bool completedDepth = true;
            
            for (const ChessMove& move : legalMoves) {
                if (isTimeUp()) {
                    completedDepth = false;
                    break;
                }
                
                searchBoard.applyMove(move);
                double eval = alphaBeta(searchBoard, depth - 1, alpha, beta, color);
                searchBoard.undoLastMove();
                
                if (timeExpired) {
                    completedDepth = false;
                    break;
                }
                
                if (eval > depthBestEval) {
                    depthBestEval = eval;
                    depthBestMove = move;
                }
                alpha = max(alpha, eval);
            }
            
            if (completedDepth) {
                // Successfully completed this depth
                bestMoveFound = depthBestMove;
                bestEvalFound = depthBestEval;
                
                double elapsed = double(clock() - searchStartTime) / CLOCKS_PER_SEC;
                cout << "  Depth " << depth << " completed: eval=" << fixed << setprecision(2) 
                     << ((color == WHITE) ? bestEvalFound : -bestEvalFound) 
                     << " (" << elapsed << "s)" << endl;
            } else {
                cout << "  Depth " << depth << " incomplete (time limit)" << endl;
                break;
            }
        }
        
        // Convert to objective (white perspective) for final output
        double objectiveEval = (color == WHITE) ? bestEvalFound : -bestEvalFound;
        
        cout << "Final evaluation: ";
        if (objectiveEval >= 100000.0) {
            cout << "#" << ((int)((objectiveEval - 100000.0) / 1000.0) + 1);
        } else if (objectiveEval <= -100000.0) {
            cout << "#-" << ((int)((-objectiveEval - 100000.0) / 1000.0) + 1);
        } else {
            cout << fixed << setprecision(2) << objectiveEval;
        }
        cout << endl;
        
        // Store for PGN export (white perspective)
        lastAIEvaluation = objectiveEval;
        
        return bestMoveFound;
    }
};

// For backward compatibility - keep old AlphaBetaAI as alias to MaterialisticAI
class AlphaBetaAI : public MaterialisticAI {
public:
    AlphaBetaAI(int depth = 3) : MaterialisticAI(depth) {}
    
    string getName() const override {
        int depth = 3;  // Default depth for display
        return "Alpha-Beta AI (depth " + to_string(depth) + ")";
    }
};

// =============================================================================
// TIME-BASED AI IMPLEMENTATIONS (Using Iterative Deepening)
// =============================================================================

// Material AI - Time-based with simple material evaluation
class MaterialAI : public ChessAI {
private:
    double timeLimit;
    clock_t searchStartTime;
    bool timeExpired;
    ChessMove bestMoveFound;
    double bestEvalFound;
    
    bool isTimeUp() const {
        double elapsed = double(clock() - searchStartTime) / CLOCKS_PER_SEC;
        return elapsed >= timeLimit;
    }
    
    // Simple material evaluation (returns integers as doubles: 1.0, 3.0, 5.0)
    double materialEval(const ChessBoard& board, Color perspective) const {
        if (board.isCheckmate(perspective)) return -100000.0;
        Color opponent = (perspective == WHITE) ? BLACK : WHITE;
        if (board.isCheckmate(opponent)) return 100000.0;
        
        double score = 0.0;
        const double PAWN_VAL = 1.0, KNIGHT_VAL = 3.0, BISHOP_VAL = 3.0, ROOK_VAL = 5.0, QUEEN_VAL = 9.0;
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                Piece p = board.getPieceAt(row, col);
                if (p.type == EMPTY) continue;
                
                double value = 0.0;
                switch(p.type) {
                    case PAWN: value = PAWN_VAL; break;
                    case KNIGHT: value = KNIGHT_VAL; break;
                    case BISHOP: value = BISHOP_VAL; break;
                    case ROOK: value = ROOK_VAL; break;
                    case QUEEN: value = QUEEN_VAL; break;
                    default: break;
                }
                
                if (p.color == perspective) score += value;
                else score -= value;
            }
        }
        return score;
    }
    
    double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, Color maximizingColor, int plyFromRoot) {
        if (timeExpired || isTimeUp()) {
            timeExpired = true;
            return 0.0;
        }
        
        if (depth == 0) {
            return materialEval(board, maximizingColor);
        }
        
        Color currentColor = board.getCurrentTurn();
        vector<ChessMove> legalMoves = board.generateLegalMoves(currentColor);
        
        if (legalMoves.empty()) {
            if (board.isInCheck(currentColor)) {
                if (currentColor == maximizingColor) {
                    return -100000.0 + plyFromRoot * 100.0;  // Getting mated, prefer slower
                } else {
                    return 100000.0 - plyFromRoot * 100.0;   // Delivering mate, prefer faster
                }
            }
            return 0.0;
        }
        
        MoveOrderer::orderMoves(legalMoves, board);
        
        if (currentColor == maximizingColor) {
            double maxEval = -100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        } else {
            double minEval = 100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }
    
public:
    MaterialAI(double timeLimitSeconds = 5.0) : timeLimit(timeLimitSeconds) {}
    
    bool usesTimeLimit() const override { return true; }
    void setTimeLimit(double seconds) override { timeLimit = seconds; }
    
    string getName() const override {
        return "Material AI (" + to_string((int)timeLimit) + "s per move)";
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) return ChessMove();
        
        searchStartTime = clock();
        timeExpired = false;
        bestMoveFound = legalMoves[0];
        bestEvalFound = -100000.0;
        
        ChessBoard searchBoard = board;
        int finalDepth = 0;
        
        // Store move evaluations for top 3 display
        struct MoveEval {
            ChessMove move;
            double eval;
            string san;
        };
        vector<MoveEval> moveEvals;
        
        // Iterative deepening
        for (int depth = 1; depth <= 20; depth++) {
            if (timeExpired || isTimeUp()) break;
            
            MoveOrderer::orderMoves(legalMoves, searchBoard);
            
            ChessMove depthBestMove = legalMoves[0];
            double depthBestEval = -100000.0;
            double alpha = -numeric_limits<double>::infinity();
            double beta = numeric_limits<double>::infinity();
            
            bool completedDepth = true;
            moveEvals.clear();
            
            for (const ChessMove& move : legalMoves) {
                if (isTimeUp()) {
                    completedDepth = false;
                    break;
                }
                
                searchBoard.applyMove(move);
                double eval = alphaBeta(searchBoard, depth - 1, alpha, beta, color, 1);
                searchBoard.undoLastMove();
                
                if (timeExpired) {
                    completedDepth = false;
                    break;
                }
                
                // Store evaluation for this move
                MoveEval me;
                me.move = move;
                me.eval = eval;
                me.san = move.toSAN(board);
                moveEvals.push_back(me);
                
                if (eval > depthBestEval) {
                    depthBestEval = eval;
                    depthBestMove = move;
                }
                alpha = max(alpha, eval);
            }
            
            if (completedDepth) {
                bestMoveFound = depthBestMove;
                bestEvalFound = depthBestEval;
                finalDepth = depth;
            } else {
                break;
            }
        }
        
        // Sort moves by evaluation (best first)
        sort(moveEvals.begin(), moveEvals.end(), [color](const MoveEval& a, const MoveEval& b) {
            return a.eval > b.eval;
        });
        
        // Show top 3 moves
        cout << "Searched to depth " << finalDepth << endl;
        cout << "Top moves:" << endl;
        for (int i = 0; i < min(3, (int)moveEvals.size()); i++) {
            double objEval = (color == WHITE) ? moveEvals[i].eval : -moveEvals[i].eval;
            cout << "  " << (i+1) << ". " << moveEvals[i].san << " (";
            
            if (objEval >= 100000.0) {
                int mateDist = (int)((objEval - 100000.0) / 100.0) + 1;
                cout << "#" << mateDist;
            } else if (objEval <= -100000.0) {
                int mateDist = (int)((-objEval - 100000.0) / 100.0) + 1;
                cout << "#-" << mateDist;
            } else {
                cout << fixed << setprecision(2) << objEval;
            }
            cout << ")" << endl;
        }
        
        double objectiveEval = (color == WHITE) ? bestEvalFound : -bestEvalFound;
        lastAIEvaluation = objectiveEval;
        return bestMoveFound;
    }
};

// Positional AI - Time-based with full positional evaluation
class PositionalAI : public ChessAI {
private:
    double timeLimit;
    clock_t searchStartTime;
    bool timeExpired;
    ChessMove bestMoveFound;
    double bestEvalFound;
    
    bool isTimeUp() const {
        double elapsed = double(clock() - searchStartTime) / CLOCKS_PER_SEC;
        return elapsed >= timeLimit;
    }
    
    // Use the full positional evaluation from PositionalAI class
    // This returns decimals like 1.35, -2.47 with piece-square bonuses
    double positionalEval(const ChessBoard& board, Color perspective) const {
        // Delegate to the existing PositionalAI's evaluation
        // We'll create a temporary PositionalAI instance to use its eval
        PositionalAI_DepthBased tempEval(3);  // Depth doesn't matter, we just need eval
        return tempEval.evaluatePosition(board, perspective);
    }
    
    double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, Color maximizingColor, int plyFromRoot) {
        if (timeExpired || isTimeUp()) {
            timeExpired = true;
            return 0.0;
        }
        
        if (depth == 0) {
            return quiesce(board, alpha, beta, maximizingColor, 3);
        }
        
        Color currentColor = board.getCurrentTurn();
        vector<ChessMove> legalMoves = board.generateLegalMoves(currentColor);
        
        if (legalMoves.empty()) {
            if (board.isInCheck(currentColor)) {
                if (currentColor == maximizingColor) {
                    return -100000.0 + plyFromRoot * 100.0;  // Getting mated, prefer slower
                } else {
                    return 100000.0 - plyFromRoot * 100.0;   // Delivering mate, prefer faster
                }
            }
            return 0.0;
        }
        
        MoveOrderer::orderMoves(legalMoves, board);
        
        if (currentColor == maximizingColor) {
            double maxEval = -100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        } else {
            double minEval = 100000.0;
            for (const ChessMove& move : legalMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double eval = alphaBeta(board, depth - 1, alpha, beta, maximizingColor, plyFromRoot + 1);
                board.undoLastMove();
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }
    
    double quiesce(ChessBoard& board, double alpha, double beta, Color maximizingColor, int depth) {
        if (timeExpired || depth == 0) {
            return positionalEval(board, maximizingColor);
        }
        
        double standPat = positionalEval(board, maximizingColor);
        
        Color currentColor = board.getCurrentTurn();
        if (currentColor == maximizingColor) {
            if (standPat >= beta) return beta;
            if (alpha < standPat) alpha = standPat;
        } else {
            if (standPat <= alpha) return alpha;
            if (beta > standPat) beta = standPat;
        }
        
        vector<ChessMove> allMoves = board.generateLegalMoves(currentColor);
        vector<ChessMove> captureMoves;
        
        for (const ChessMove& move : allMoves) {
            Piece target = board.getPieceAt(move.toRow, move.toCol);
            if (target.type != EMPTY || move.promotionPiece != EMPTY) {
                captureMoves.push_back(move);
            }
        }
        
        if (captureMoves.empty()) return standPat;
        
        MoveOrderer::orderMoves(captureMoves, board);
        
        if (currentColor == maximizingColor) {
            for (const ChessMove& move : captureMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }
            return alpha;
        } else {
            for (const ChessMove& move : captureMoves) {
                if (timeExpired) break;
                board.applyMove(move);
                double score = quiesce(board, alpha, beta, maximizingColor, depth - 1);
                board.undoLastMove();
                
                if (score <= alpha) return alpha;
                if (score < beta) beta = score;
            }
            return beta;
        }
    }
    
public:
    PositionalAI(double timeLimitSeconds = 5.0) : timeLimit(timeLimitSeconds) {}
    
    bool usesTimeLimit() const override { return true; }
    void setTimeLimit(double seconds) override { timeLimit = seconds; }
    
    string getName() const override {
        return "Positional AI (" + to_string((int)timeLimit) + "s per move)";
    }
    
    ChessMove getBestMove(const ChessBoard& board, Color color) override {
        vector<ChessMove> legalMoves = board.generateLegalMoves(color);
        if (legalMoves.empty()) return ChessMove();
        
        searchStartTime = clock();
        timeExpired = false;
        bestMoveFound = legalMoves[0];
        bestEvalFound = -100000.0;
        
        ChessBoard searchBoard = board;
        int finalDepth = 0;
        
        // Store move evaluations for top 3 display
        struct MoveEval {
            ChessMove move;
            double eval;
            string san;
        };
        vector<MoveEval> moveEvals;
        
        // Iterative deepening
        for (int depth = 1; depth <= 20; depth++) {
            if (timeExpired || isTimeUp()) break;
            
            MoveOrderer::orderMoves(legalMoves, searchBoard);
            
            ChessMove depthBestMove = legalMoves[0];
            double depthBestEval = -100000.0;
            double alpha = -numeric_limits<double>::infinity();
            double beta = numeric_limits<double>::infinity();
            
            bool completedDepth = true;
            moveEvals.clear();
            
            for (const ChessMove& move : legalMoves) {
                if (isTimeUp()) {
                    completedDepth = false;
                    break;
                }
                
                searchBoard.applyMove(move);
                double eval = alphaBeta(searchBoard, depth - 1, alpha, beta, color, 1);
                searchBoard.undoLastMove();
                
                if (timeExpired) {
                    completedDepth = false;
                    break;
                }
                
                // Store evaluation for this move
                MoveEval me;
                me.move = move;
                me.eval = eval;
                me.san = move.toSAN(board);
                moveEvals.push_back(me);
                
                if (eval > depthBestEval) {
                    depthBestEval = eval;
                    depthBestMove = move;
                }
                alpha = max(alpha, eval);
            }
            
            if (completedDepth) {
                bestMoveFound = depthBestMove;
                bestEvalFound = depthBestEval;
                finalDepth = depth;
            } else {
                break;
            }
        }
        
        // Sort moves by evaluation (best first)
        sort(moveEvals.begin(), moveEvals.end(), [color](const MoveEval& a, const MoveEval& b) {
            return a.eval > b.eval;
        });
        
        // Show top 3 moves
        cout << "Searched to depth " << finalDepth << endl;
        cout << "Top moves:" << endl;
        for (int i = 0; i < min(3, (int)moveEvals.size()); i++) {
            double objEval = (color == WHITE) ? moveEvals[i].eval : -moveEvals[i].eval;
            cout << "  " << (i+1) << ". " << moveEvals[i].san << " (";
            
            if (objEval >= 100000.0) {
                int mateDist = (int)((objEval - 100000.0) / 100.0) + 1;
                cout << "#" << mateDist;
            } else if (objEval <= -100000.0) {
                int mateDist = (int)((-objEval - 100000.0) / 100.0) + 1;
                cout << "#-" << mateDist;
            } else {
                cout << fixed << setprecision(2) << objEval;
            }
            cout << ")" << endl;
        }
        
        double objectiveEval = (color == WHITE) ? bestEvalFound : -bestEvalFound;
        lastAIEvaluation = objectiveEval;
        return bestMoveFound;
    }
};

int main() {
    srand(time(0));  // Seed random number generator
    
    ChessBoard chess;
    
    cout << "=== Chess Game ===" << endl;
    cout << "\nSelect White Player:" << endl;
    cout << "  1. Human" << endl;
    cout << "  2. Random AI" << endl;
    cout << "  3. Material AI (time-based, simple evaluation)" << endl;
    cout << "  4. Positional AI (time-based, advanced evaluation)" << endl;
    cout << "Choice (1-4): ";
    
    int whiteChoice;
    cin >> whiteChoice;
    cin.ignore();
    
    double whiteTimeLimit = 5.0;
    if (whiteChoice >= 3 && whiteChoice <= 4) {
        cout << "Select time limit for White in seconds (1-60, recommended 3-10): ";
        cin >> whiteTimeLimit;
        cin.ignore();
        if (whiteTimeLimit < 1) whiteTimeLimit = 1;
        if (whiteTimeLimit > 60) whiteTimeLimit = 60;
    }
    
    cout << "\nSelect Black Player:" << endl;
    cout << "  1. Human" << endl;
    cout << "  2. Random AI" << endl;
    cout << "  3. Material AI (time-based, simple evaluation)" << endl;
    cout << "  4. Positional AI (time-based, advanced evaluation)" << endl;
    cout << "Choice (1-4): ";
    
    int blackChoice;
    cin >> blackChoice;
    cin.ignore();
    
    double blackTimeLimit = 5.0;
    if (blackChoice >= 3 && blackChoice <= 4) {
        cout << "Select time limit for Black in seconds (1-60, recommended 3-10): ";
        cin >> blackTimeLimit;
        cin.ignore();
        if (blackTimeLimit < 1) blackTimeLimit = 1;
        if (blackTimeLimit > 60) blackTimeLimit = 60;
    }
    
    // Create AI players
    ChessAI* whiteAI = nullptr;
    ChessAI* blackAI = nullptr;
    string whitePlayerName = "Human";
    string blackPlayerName = "Human";
    
    if (whiteChoice == 2) {
        whiteAI = new RandomAI();
        whitePlayerName = whiteAI->getName();
    } else if (whiteChoice == 3) {
        whiteAI = new MaterialAI(whiteTimeLimit);
        whitePlayerName = whiteAI->getName();
    } else if (whiteChoice == 4) {
        whiteAI = new PositionalAI(whiteTimeLimit);
        whitePlayerName = whiteAI->getName();
    }
    
    if (blackChoice == 2) {
        blackAI = new RandomAI();
        blackPlayerName = blackAI->getName();
    } else if (blackChoice == 3) {
        blackAI = new MaterialAI(blackTimeLimit);
        blackPlayerName = blackAI->getName();
    } else if (blackChoice == 4) {
        blackAI = new PositionalAI(blackTimeLimit);
        blackPlayerName = blackAI->getName();
    }
    
    cout << "\n=== Game Start ===" << endl;
    cout << "White: " << whitePlayerName << endl;
    cout << "Black: " << blackPlayerName << endl;
    
    // Display complexity information
    cout << "\n** Thinking Time Scaling **" << endl;
    cout << "Approximate time increases per depth level:" << endl;
    cout << "  Depth 1: <0.1s  (instant)" << endl;
    cout << "  Depth 2: <0.5s  (very fast)" << endl;
    cout << "  Depth 3: 1-3s   (fast)" << endl;
    cout << "  Depth 4: 5-15s  (moderate)" << endl;
    cout << "  Depth 5: 30-90s (slow)" << endl;
    cout << "  Depth 6: 2-10min (very slow)" << endl;
    cout << "Note: Times vary based on position complexity and move ordering.\n" << endl;
    
    cout << "\nCommands:" << endl;
    cout << "  Enter moves in algebraic notation (e.g., e4, Nf3, O-O)" << endl;
    cout << "  'f' or 'flip' - Flip board" << endl;
    cout << "  'u' or 'undo' - Undo last move" << endl;
    cout << "  'save' - Save game as PGN" << endl;
    cout << "  'q' or 'quit' - Exit game" << endl;
    
    chess.display();
    
    string gameResult = "*";  // Ongoing game
    bool gameEnded = false;
    
    while (!gameEnded) {
        Color currentTurn = chess.getCurrentTurn();
        ChessAI* currentAI = (currentTurn == WHITE) ? whiteAI : blackAI;
        
        if (currentAI != nullptr) {
            // AI move
            cout << "\n" << (currentTurn == WHITE ? whitePlayerName : blackPlayerName) << " is thinking..." << endl;
            
            auto startTime = clock();
            ChessMove aiMove = currentAI->getBestMove(chess, currentTurn);
            auto endTime = clock();
            
            double thinkTime = double(endTime - startTime) / CLOCKS_PER_SEC;
            
            if (!aiMove.isValid()) {
                // No valid moves - game should already be over
                cout << "No legal moves available." << endl;
                if (chess.isCheckmate(currentTurn)) {
                    gameResult = (currentTurn == WHITE) ? "0-1" : "1-0";
                } else {
                    gameResult = "1/2-1/2";  // Stalemate
                }
                gameEnded = true;
                continue;
            }
            
            // Generate SAN notation before making the move
            string sanMove = aiMove.toSAN(chess);
            
            string moveStr = aiMove.toAlgebraic();
            cout << "AI plays: " << sanMove << " (thought for " << fixed << setprecision(2) << thinkTime << "s)" << endl;
            
            if (chess.makeMove(moveStr)) {
                // Store SAN and evaluation (white perspective)
                chess.setLastMoveEvaluation(lastAIEvaluation, currentTurn);
                chess.setLastMoveSAN(sanMove);
                
                chess.display();
                
                // Check for game end
                if (chess.isDraw()) {
                    if (chess.isThreefoldRepetition()) {
                        cout << "Draw by threefold repetition!" << endl;
                    } else if (chess.isFiftyMoveRule()) {
                        cout << "Draw by fifty-move rule!" << endl;
                    } else if (chess.isInsufficientMaterial()) {
                        cout << "Draw by insufficient material!" << endl;
                    }
                    gameResult = "1/2-1/2";
                    gameEnded = true;
                } else if (chess.isCheckmate(chess.getCurrentTurn())) {
                    gameResult = (currentTurn == WHITE) ? "1-0" : "0-1";
                    gameEnded = true;
                } else if (chess.isStalemate(chess.getCurrentTurn())) {
                    cout << "Stalemate!" << endl;
                    gameResult = "1/2-1/2";
                    gameEnded = true;
                }
            } else {
                cout << "AI made an invalid move!" << endl;
                break;
            }
        } else {
            // Human move
            cout << "\nEnter move: ";
            string input;
            getline(cin, input);
            
            // Trim whitespace
            input.erase(0, input.find_first_not_of(" \t"));
            input.erase(input.find_last_not_of(" \t") + 1);
            
            if (input.empty()) continue;
            
            // Convert to lowercase for command checking
            string lowerInput = input;
            transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
            
            if (lowerInput == "q" || lowerInput == "quit" || lowerInput == "exit") {
                cout << "Game ended by user." << endl;
                gameResult = "*";
                break;
            }
            
            if (lowerInput == "f" || lowerInput == "flip") {
                chess.flipBoard();
                chess.display();
                continue;
            }
            
            if (lowerInput == "u" || lowerInput == "undo") {
                // Undo both human and AI move (if playing against AI)
                bool anyAI = (whiteAI != nullptr || blackAI != nullptr);
                if (anyAI) {
                    if (chess.undoHumanMove()) {
                        chess.display();
                    } else {
                        cout << "No moves to undo!" << endl;
                    }
                } else {
                    // Human vs human, just undo one move
                    if (chess.undoMove()) {
                        chess.display();
                    } else {
                        cout << "No moves to undo!" << endl;
                    }
                }
                continue;
            }
            
            if (lowerInput == "save") {
                // Create games directory if it doesn't exist
                system("mkdir -p games");
                
                // Generate filename with timestamp
                time_t now = time(0);
                tm* ltm = localtime(&now);
                stringstream filename;
                filename << "games/chess_"
                         << (1900 + ltm->tm_year)
                         << setfill('0') << setw(2) << (1 + ltm->tm_mon)
                         << setfill('0') << setw(2) << ltm->tm_mday << "_"
                         << setfill('0') << setw(2) << ltm->tm_hour
                         << setfill('0') << setw(2) << ltm->tm_min
                         << setfill('0') << setw(2) << ltm->tm_sec
                         << ".pgn";
                
                string pgnContent = chess.exportPGN(whitePlayerName, blackPlayerName, gameResult);
                ofstream pgnFile(filename.str());
                pgnFile << pgnContent;
                pgnFile.close();
                
                cout << "Game saved to: " << filename.str() << endl;
                continue;
            }
            
            if (chess.makeMove(input)) {
                // Store the SAN notation (human input is already in SAN format)
                chess.setLastMoveSAN(input);
                
                chess.display();
                
                // Check for game end
                if (chess.isDraw()) {
                    if (chess.isThreefoldRepetition()) {
                        cout << "Draw by threefold repetition!" << endl;
                    } else if (chess.isFiftyMoveRule()) {
                        cout << "Draw by fifty-move rule!" << endl;
                    } else if (chess.isInsufficientMaterial()) {
                        cout << "Draw by fifty-move rule!" << endl;
                    }
                    gameResult = "1/2-1/2";
                    gameEnded = true;
                } else if (chess.isCheckmate(chess.getCurrentTurn())) {
                    gameResult = (currentTurn == WHITE) ? "1-0" : "0-1";
                    gameEnded = true;
                } else if (chess.isStalemate(chess.getCurrentTurn())) {
                    cout << "Stalemate!" << endl;
                    gameResult = "1/2-1/2";
                    gameEnded = true;
                }
            } else {
                cout << "Invalid move! Try again." << endl;
            }
        }
    }
    
    // Final save prompt - always offer if game ended properly
    if (gameEnded) {
        cout << "\nGame Over! Result: " << gameResult << endl;
        cout << "Save game? (y/n): ";
        string response;
        getline(cin, response);
        
        if (response == "y" || response == "Y" || response == "yes" || response == "YES") {
            system("mkdir -p games");
            
            time_t now = time(0);
            tm* ltm = localtime(&now);
            stringstream filename;
            filename << "games/chess_"
                     << (1900 + ltm->tm_year)
                     << setfill('0') << setw(2) << (1 + ltm->tm_mon)
                     << setfill('0') << setw(2) << ltm->tm_mday << "_"
                     << setfill('0') << setw(2) << ltm->tm_hour
                     << setfill('0') << setw(2) << ltm->tm_min
                     << setfill('0') << setw(2) << ltm->tm_sec
                     << ".pgn";
            
            string pgnContent = chess.exportPGN(whitePlayerName, blackPlayerName, gameResult);
            ofstream pgnFile(filename.str());
            pgnFile << pgnContent;
            pgnFile.close();
            
            cout << "Game saved to: " << filename.str() << endl;
        }
    } else if (gameResult != "*") {
        // Game ended abnormally but still offer save
        cout << "\nGame ended. Result: " << gameResult << endl;
        cout << "Save game? (y/n): ";
        string response;
        getline(cin, response);
        
        if (response == "y" || response == "Y" || response == "yes" || response == "YES") {
            system("mkdir -p games");
            
            time_t now = time(0);
            tm* ltm = localtime(&now);
            stringstream filename;
            filename << "games/chess_"
                     << (1900 + ltm->tm_year)
                     << setfill('0') << setw(2) << (1 + ltm->tm_mon)
                     << setfill('0') << setw(2) << ltm->tm_mday << "_"
                     << setfill('0') << setw(2) << ltm->tm_hour
                     << setfill('0') << setw(2) << ltm->tm_min
                     << setfill('0') << setw(2) << ltm->tm_sec
                     << ".pgn";
            
            string pgnContent = chess.exportPGN(whitePlayerName, blackPlayerName, gameResult);
            ofstream pgnFile(filename.str());
            pgnFile << pgnContent;
            pgnFile.close();
            
            cout << "Game saved to: " << filename.str() << endl;
        }
    }
    
    // Cleanup
    delete whiteAI;
    delete blackAI;
    
    return 0;
}
