#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <utility>

using namespace std;

enum PieceType { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color { NONE, WHITE, BLACK };

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
    
    Move(int fr, int fc, int tr, int tc, Piece cap, bool ep, int epCol, bool castle = false) 
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), capturedPiece(cap), 
          wasEnPassant(ep), prevEnPassantCol(epCol), wasCastling(castle) {}
};

class ChessBoard {
private:
    Piece board[8][8];
    Color currentTurn;
    bool flipped;
    int enPassantCol;  // -1 if no en passant available, otherwise the column of the pawn that can be captured
    vector<Move> moveHistory;
    
    // Castling rights
    bool whiteKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackKingMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;
    
public:
    ChessBoard() : currentTurn(WHITE), flipped(false), enPassantCol(-1),
                   whiteKingMoved(false), whiteRookKingsideMoved(false), whiteRookQueensideMoved(false),
                   blackKingMoved(false), blackRookKingsideMoved(false), blackRookQueensideMoved(false) {
        initializeBoard();
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
        
        Move lastMove = moveHistory.back();
        moveHistory.pop_back();
        
        // Restore the piece to its original position
        board[lastMove.fromRow][lastMove.fromCol] = board[lastMove.toRow][lastMove.toCol];
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
                            moveHistory.push_back(Move(fromRow, fromCol, toRow, toCol, capturedPiece, isEnPassant, prevEnPassant, isCastling));
                            
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
                            
                            currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
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
            moveHistory.push_back(Move(fromRow, fromCol, toRow, toCol, capturedPiece, isEnPassant, prevEnPassant, isCastling));
            
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
            return true;
        }
        
        return false;
    }
    
    Color getCurrentTurn() const {
        return currentTurn;
    }
};

int main() {
    ChessBoard chess;
    string input;
    
    cout << "=== Chess Game ===" << endl;
    cout << "Enter moves in standard algebraic notation:" << endl;
    cout << "  Examples: e4, Nf3, Bxc4, O-O, Qh5+" << endl;
    cout << "  Long notation also works: e2e4 or e2-e4" << endl;
    cout << "Commands:" << endl;
    cout << "  'f' or 'flip' - Flip board" << endl;
    cout << "  'u' or 'undo' - Undo last move" << endl;
    cout << "  'q' or 'quit' - Exit game" << endl;
    
    chess.display();
    
    while (true) {
        cout << "\nEnter move: ";
        getline(cin, input);
        
        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if (input.empty()) continue;
        
        // Convert to lowercase for command checking
        string lowerInput = input;
        transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
        
        if (lowerInput == "q" || lowerInput == "quit" || lowerInput == "exit") {
            cout << "Thanks for playing!" << endl;
            break;
        }
        
        if (lowerInput == "f" || lowerInput == "flip") {
            chess.flipBoard();
            chess.display();
            continue;
        }
        
        if (lowerInput == "u" || lowerInput == "undo") {
            if (chess.undoMove()) {
                chess.display();
            } else {
                cout << "No moves to undo!" << endl;
            }
            continue;
        }
        
        if (chess.makeMove(input)) {
            chess.display();
        } else {
            cout << "Invalid move! Try again." << endl;
        }
    }
    
    return 0;
}
