# Chess Engine - Complete Code Documentation

## Table of Contents
1. [Overview](#overview)
2. [Data Structures](#data-structures)
3. [Core Classes](#core-classes)
4. [AI System](#ai-system)
5. [Search Algorithms](#search-algorithms)
6. [Game Flow](#game-flow)
7. [Key Functions](#key-functions)

---

## Overview

This is a complete chess engine with:
- Full chess rules implementation (castling, en passant, promotion, etc.)
- Multiple AI opponents with different strategies
- Time-based iterative deepening search
- PGN export for game analysis
- Draw detection (threefold repetition, fifty-move rule, insufficient material)

**Architecture:** Single-file C++ application (~3200 lines)
**Search:** Alpha-beta pruning with quiescence search
**Evaluation:** Material count + positional factors + pawn structure

---

## Data Structures

### `Piece`
```cpp
struct Piece {
    PieceType type;  // PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, or EMPTY
    Color color;     // WHITE or BLACK
}
```
**Purpose:** Represents a single chess piece
**Usage:** `board[row][col]` returns a Piece

---

### `ChessMove`
```cpp
struct ChessMove {
    int fromRow, fromCol;    // Starting square
    int toRow, toCol;        // Destination square
    PieceType promotionPiece; // QUEEN/ROOK/BISHOP/KNIGHT if promotion, else EMPTY
}
```
**Purpose:** Represents a chess move
**Methods:**
- `toAlgebraic()` - Converts to "e2e4" or "e7e8q" format
- `toSAN()` - Converts to standard algebraic notation like "Nf3" or "Bxc4"

**Example:**
```cpp
ChessMove move(6, 4, 4, 4);  // e2 to e4 (pawn advance)
ChessMove promo(1, 2, 0, 2, QUEEN);  // c7 to c8=Q (promotion)
```

---

### `Move` (History Record)
```cpp
struct Move {
    int fromRow, fromCol, toRow, toCol;
    Piece capturedPiece;      // What was captured (or EMPTY)
    bool wasEnPassant;        // Was this an en passant capture?
    int prevEnPassantCol;     // Previous en passant state
    bool wasCastling;         // Was this a castling move?
    double evaluation;        // AI's evaluation (for PGN export)
    string sanNotation;       // Move in SAN format
    int halfMoveClock;        // For fifty-move rule
}
```
**Purpose:** Complete record of a move for undo functionality
**Usage:** Stored in `moveHistory` vector

---

## Core Classes

### `ChessBoard`
**Purpose:** Manages the entire chess game state and rules

#### Key Data Members:
```cpp
Piece board[8][8];              // The chess board
Color currentTurn;              // WHITE or BLACK
int enPassantCol;               // Column where en passant is possible (-1 if none)
bool whiteKingMoved;            // Castling rights tracking
bool whiteRookKingsideMoved;
bool whiteRookQueensideMoved;
bool blackKingMoved;
bool blackRookKingsideMoved;
bool blackRookQueensideMoved;
int halfMoveClock;              // For fifty-move rule (resets on pawn move/capture)
map<string, int> positionHistory; // For threefold repetition detection
vector<Move> moveHistory;       // All moves played (for undo)
```

#### Core Methods:

**`void initialize()`**
- Sets up the starting position
- Initializes all castling rights
- Sets white to move

**`bool makeMove(const string& move)`**
- Parses algebraic notation (e.g., "e4", "Nf3", "O-O")
- Validates the move
- Executes the move on the board
- Updates game state (castling rights, en passant, turn)
- **CRITICAL:** Handles pawn promotion (converts PAWN to QUEEN/ROOK/etc)
- Adds to move history
- Returns true if successful

**`bool applyMove(const ChessMove& move)`**
- Used by AI for search
- Converts ChessMove to string and calls makeMove()
- Returns true if successful

**`bool undoMove()`**
- Reverses the last move
- Restores captured pieces
- **CRITICAL:** Restores PAWN if a promotion was undone
- Restores castling rights by scanning history
- Returns true if successful

**`vector<ChessMove> generateLegalMoves(Color color)`**
- Generates ALL legal moves for a color
- Checks each possible destination square
- Validates piece movement patterns
- **CRITICAL:** Generates 4 separate moves for promotions (Q, R, B, N)
- Filters out moves that leave king in check
- Returns vector of legal ChessMove objects

**`bool isValidMove(int fromRow, fromCol, int toRow, toCol)`**
- Checks if a move is legal
- Validates piece movement patterns
- Checks for path obstructions
- Handles special moves (castling, en passant)
- Verifies move doesn't leave king in check

**`bool isInCheck(Color color)`**
- Checks if the given color's king is under attack
- Used for check/checkmate detection
- Used to validate castling legality

**`bool isCheckmate(Color color)`**
- Returns true if color is in checkmate
- Checks: in check AND no legal moves

**`bool isStalemate(Color color)`**
- Returns true if color is stalemated
- Checks: NOT in check AND no legal moves

**`bool isDraw()`**
- Returns true if game is drawn
- Checks threefold repetition, fifty-move rule, insufficient material

**`double evaluatePosition(Color perspective)`**
- Simple material evaluation
- P=1, N=3, B=3, R=5, Q=9
- Returns positive if perspective is winning
- Used by MaterialAI

**`string exportPGN()`**
- Exports game to Portable Game Notation
- Includes date, players, result
- Includes move evaluations as comments
- Used for game analysis

---

### `PositionHash`
**Purpose:** Creates unique hash strings for board positions

```cpp
static string hashPosition(const ChessBoard& board) {
    // Creates string representation of board + turn
    // Used for threefold repetition detection
}
```

**How it works:**
- Converts board to string like "rnbqkbnr/pppppppp/..."
- Adds current turn
- Returns unique identifier for position

---

### `MoveOrderer`
**Purpose:** Improves alpha-beta search by ordering moves

```cpp
static void orderMoves(vector<ChessMove>& moves, const ChessBoard& board) {
    // Sorts moves to improve pruning
    // Priority: Captures > Checks > Other moves
}
```

**Why it matters:**
- Better move ordering = more alpha-beta cutoffs
- Can reduce search tree by 50%+
- Dramatically improves search depth

---

## AI System

### Base Class: `ChessAI`
```cpp
class ChessAI {
    virtual ChessMove getBestMove(const ChessBoard& board, Color color) = 0;
    virtual string getName() const = 0;
    virtual bool usesTimeLimit() const { return false; }
}
```
**Purpose:** Interface for all AI implementations

---

### `RandomAI`
**Purpose:** Makes random legal moves

```cpp
ChessMove getBestMove(const ChessBoard& board, Color color) {
    vector<ChessMove> legalMoves = board.generateLegalMoves(color);
    return legalMoves[rand() % legalMoves.size()];
}
```

**Use case:** Testing, debugging, or just for fun

---

### `MaterialAI` (Time-based)
**Purpose:** Uses simple material evaluation with iterative deepening

#### Evaluation Function:
```cpp
double materialEval(const ChessBoard& board, Color perspective) {
    // Count pieces: P=1, N=3, B=3, R=5, Q=9
    // Returns difference from perspective's view
}
```

#### Search Strategy:
1. Start at depth 1
2. Search all moves to depth 1
3. If time remaining, increase to depth 2
4. Repeat until time runs out
5. Return best move from deepest completed search

**Output:**
```
Searched to depth 5
Top moves:
  1. e4 (1.00)
  2. d4 (0.80)
  3. Nf3 (0.60)
```

**Characteristics:**
- Fast (simple evaluation)
- Returns integer scores (1.00, 3.00, 5.00)
- Good for learning and quick games

---

### `PositionalAI` (Time-based)
**Purpose:** Advanced evaluation with positional factors

#### Evaluation Function:
```cpp
double positionalEval(const ChessBoard& board, Color perspective) {
    double score = 0.0;
    
    // 1. Material (with bishop pair bonus)
    score += material;  // P=1, N=3, B=3.2, R=5, Q=9
    
    // 2. Piece-square tables
    score += pawnTable[row][col] / 10.0;    // Encourage central pawns
    score += knightTable[row][col] / 10.0;  // Knights good in center
    score += bishopTable[row][col] / 10.0;  // Bishops on long diagonals
    
    // 3. Pawn structure
    score -= doubledPawns * 0.5;    // Penalize doubled pawns
    score -= isolatedPawns * 0.5;   // Penalize isolated pawns
    score += passedPawns * 0.5;     // Reward passed pawns
    
    // 4. Piece development
    score += developedMinorPieces * 0.15;  // Reward development
    score -= earlyQueenDevelopment * 1.0;  // Penalize early queen
    
    // 5. Rook positioning
    score += rookOnOpenFile * 0.5;
    score += rookOnSemiOpenFile * 0.25;
    
    // 6. King safety
    score += pawnShield * 0.1;  // Reward pawns protecting king
    
    // 7. Mobility
    score += (myMoves - opponentMoves) * 0.1;
    
    // 8. Center control
    score += centerSquareControl * 0.3;
    
    return score;
}
```

**Search Strategy:**
- Uses same iterative deepening as MaterialAI
- **PLUS:** Quiescence search to avoid "horizon effect"

#### Quiescence Search:
```cpp
double quiesce(ChessBoard& board, double alpha, double beta, Color color, int depth) {
    // Evaluate position
    double standPat = positionalEval(board, color);
    
    // Only search captures/promotions
    for (capture_moves) {
        // Recursively search captures
        // Stops when position is "quiet" (no more captures)
    }
}
```

**Why quiesce matters:**
- Prevents the "horizon effect" where AI overlooks immediate tactics
- Example: Without quiesce, AI might think "I'll capture that queen!" but miss the recapture

**Characteristics:**
- Slower (complex evaluation)
- Returns decimal scores (1.35, -2.47, 0.63)
- Much stronger tactical/positional play

---

## Search Algorithms

### Alpha-Beta Pruning

```cpp
double alphaBeta(ChessBoard& board, int depth, double alpha, double beta, 
                 Color maximizingColor, int plyFromRoot) {
    
    // Terminal conditions
    if (depth == 0) return evaluate(board, maximizingColor);
    
    vector<ChessMove> moves = board.generateLegalMoves(currentColor);
    
    // Checkmate detection
    if (moves.empty()) {
        if (isInCheck) {
            if (we_are_in_checkmate) {
                return -100000 + plyFromRoot * 100;  // Prefer delaying mate
            } else {
                return 100000 - plyFromRoot * 100;   // Prefer faster mate
            }
        }
        return 0;  // Stalemate
    }
    
    // Search all moves
    if (maximizing) {
        for (each move) {
            board.applyMove(move);
            eval = alphaBeta(board, depth-1, alpha, beta, color, plyFromRoot+1);
            board.undoMove();
            
            alpha = max(alpha, eval);
            if (beta <= alpha) break;  // Beta cutoff (pruning!)
        }
        return alpha;
    } else {
        for (each move) {
            board.applyMove(move);
            eval = alphaBeta(board, depth-1, alpha, beta, color, plyFromRoot+1);
            board.undoMove();
            
            beta = min(beta, eval);
            if (beta <= alpha) break;  // Alpha cutoff (pruning!)
        }
        return beta;
    }
}
```

**Key Concepts:**

**Alpha:** Best score maximizer can guarantee
**Beta:** Best score minimizer can guarantee
**Pruning:** If beta <= alpha, remaining moves won't change outcome

**Example:**
```
Position: White to move
Alpha = -infinity (worst white can do)
Beta = +infinity (best black can do)

Try move e4:
  Black responds with e5: eval = +0.5
  Alpha = 0.5
  
Try move d4:
  Black responds with d5: eval = +0.3
  Black responds with Nf6: eval = -0.2
  Beta = -0.2
  
  Since beta (-0.2) < alpha (0.5):
    STOP searching moves after d4!
    We know white can do better with e4
```

**Pruning benefit:** Can skip 50-90% of positions!

---

### Iterative Deepening

```cpp
for (depth = 1; depth <= 20; depth++) {
    if (time_expired) break;
    
    bestMove = searchToDepth(depth);
    
    if (completed_depth) {
        save_best_move();
        finalDepth = depth;
    }
}

return bestMove;  // From deepest completed search
```

**Benefits:**
1. **Time control:** Stop exactly when time runs out
2. **Move ordering:** Use previous depth results to order moves
3. **Anytime algorithm:** Always have a valid move
4. **Progressive deepening:** Simple positions search deeper

**Example:**
```
Time limit: 5 seconds
Depth 1: 0.01s (complete) - best move: e4
Depth 2: 0.05s (complete) - best move: e4
Depth 3: 0.2s (complete)  - best move: Nf3
Depth 4: 1.0s (complete)  - best move: Nf3
Depth 5: 4.5s (complete)  - best move: d4
Depth 6: 3.0s (incomplete, time expired)
Return: d4 (from depth 5)
```

---

## Game Flow

### Main Game Loop

```cpp
int main() {
    // 1. Setup
    ChessBoard chess;
    chess.initialize();
    
    // 2. Player selection
    cout << "Select White Player: 1=Human, 2=Random, 3=Material, 4=Positional";
    // Create AI objects based on choice
    
    // 3. Game loop
    while (!gameEnded) {
        chess.display();
        
        if (currentPlayer == human) {
            // Get move from keyboard
            string input;
            cin >> input;
            
            if (input == "q") quit();
            if (input == "save") exportPGN();
            if (input == "undo") chess.undoHumanMove();  // Undo 2 moves
            
            chess.makeMove(input);
        } else {
            // Get move from AI
            ChessMove move = ai->getBestMove(chess, currentColor);
            chess.applyMoveWithSAN(move, san, evaluation);
        }
        
        // Check for game end
        if (chess.isCheckmate(currentColor)) {
            gameResult = (currentColor == WHITE) ? "0-1" : "1-0";
            gameEnded = true;
        }
        if (chess.isDraw()) {
            gameResult = "1/2-1/2";
            gameEnded = true;
        }
    }
    
    // 4. Game over
    cout << "Game ended. Save? (y/n)";
    // Offer to save PGN
}
```

---

## Key Functions

### `parseSquare(const string& square, int& row, int& col)`
**Purpose:** Converts "e4" to row=4, col=4

```cpp
bool parseSquare(const string& square, int& row, int& col) {
    if (square.length() != 2) return false;
    col = square[0] - 'a';  // 'a' = 0, 'b' = 1, etc.
    row = 8 - (square[1] - '0');  // '1' = row 7, '8' = row 0
    return (row >= 0 && row < 8 && col >= 0 && col < 8);
}
```

---

### `isPathClear(int fromRow, fromCol, int toRow, toCol)`
**Purpose:** Checks if path between squares is empty (for bishops, rooks, queens)

```cpp
bool isPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDir = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int colDir = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
    
    int r = fromRow + rowDir;
    int c = fromCol + colDir;
    
    while (r != toRow || c != toCol) {
        if (board[r][c].type != EMPTY) return false;  // Blocked!
        r += rowDir;
        c += colDir;
    }
    
    return true;
}
```

---

### `wouldBeInCheck(int fromRow, fromCol, int toRow, toCol, Color color)`
**Purpose:** Checks if making this move would leave king in check

```cpp
bool wouldBeInCheck(...) {
    // 1. Save current state
    Piece movedPiece = board[fromRow][fromCol];
    Piece capturedPiece = board[toRow][toCol];
    
    // 2. Make the move temporarily
    board[toRow][toCol] = movedPiece;
    board[fromRow][fromCol] = EMPTY;
    
    // 3. Check if king is attacked
    bool inCheck = isInCheck(color);
    
    // 4. Undo the temporary move
    board[fromRow][fromCol] = movedPiece;
    board[toRow][toCol] = capturedPiece;
    
    return inCheck;
}
```

---

### `canCastle(int fromRow, int fromCol, int toRow, int toCol)`
**Purpose:** Validates castling move

```cpp
bool canCastle(...) {
    // 1. Check piece is king
    if (board[fromRow][fromCol].type != KING) return false;
    
    // 2. Check king hasn't moved
    if (whiteKingMoved) return false;  // (for white)
    
    // 3. Check rook hasn't moved
    if (isKingside && whiteRookKingsideMoved) return false;
    
    // 4. Check squares between are empty
    if (board[7][5].type != EMPTY || board[7][6].type != EMPTY) return false;
    
    // 5. Check king not in check
    if (isInCheck(WHITE)) return false;
    
    // 6. Check king doesn't pass through check
    if (isSquareAttacked(7, 5, BLACK)) return false;
    
    // 7. Check king doesn't land in check
    if (isSquareAttacked(7, 6, BLACK)) return false;
    
    return true;
}
```

---

### `isInsufficientMaterial()`
**Purpose:** Detects if position has enough material to checkmate

```cpp
bool isInsufficientMaterial() {
    // Count all pieces
    int whitePawns, blackPawns, whiteKnights, blackKnights, ...;
    
    // Any pawns/rooks/queens = sufficient material
    if (whitePawns > 0 || blackPawns > 0) return false;
    if (whiteRooks > 0 || blackRooks > 0) return false;
    if (whiteQueens > 0 || blackQueens > 0) return false;
    
    // K vs K
    if (whiteKnights + whiteBishops == 0 && 
        blackKnights + blackBishops == 0) return true;
    
    // K+minor vs K
    if (whiteKnights + whiteBishops == 1 && 
        blackKnights + blackBishops == 0) return true;
    
    // K+B vs K+B (same color bishops)
    if (whiteBishops == 1 && blackBishops == 1 &&
        whiteKnights == 0 && blackKnights == 0) {
        // Check if bishops on same color squares
        // ...
    }
    
    return false;
}
```

---

## Performance Characteristics

### Typical Search Depths (5 second time limit):

| Position Complexity | MaterialAI | PositionalAI |
|---------------------|------------|--------------|
| Opening (30 moves)  | Depth 6-7  | Depth 5-6    |
| Middlegame (35 moves)| Depth 5-6  | Depth 4-5    |
| Endgame (10 moves)  | Depth 8-10 | Depth 7-9    |

### Nodes Searched (approximate):

| Depth | Nodes (no pruning) | Nodes (alpha-beta) | Speedup |
|-------|-------------------|-------------------|---------|
| 4     | 1,000,000         | 10,000           | 100x    |
| 5     | 35,000,000        | 100,000          | 350x    |
| 6     | 1,200,000,000     | 500,000          | 2400x   |

**Alpha-beta pruning is CRITICAL for performance!**

---

## Common Debugging Scenarios

### "AI doesn't see checkmate in 1"
**Check:** Is the AI searching deep enough?
```cpp
cout << "Searched to depth " << finalDepth << endl;
```
If depth is 0 or 1, increase time limit

### "AI makes illegal move"
**Check:** `generateLegalMoves()` validation
**Fix:** Ensure `wouldBeInCheck()` is called for each move

### "Undo doesn't work"
**Check:** `undoMove()` restoration
**Common issue:** Promotion - must restore PAWN not promoted piece

### "Evaluation seems wrong"
**Check:** Perspective (white vs black)
```cpp
// Evaluation is ALWAYS from perspective's point of view
// +5 = perspective is up 5 points
// -5 = perspective is down 5 points
```

### "AI is too slow"
**Solutions:**
1. Reduce time limit
2. Use MaterialAI instead of PositionalAI
3. Add transposition tables (future optimization)

---

## Future Optimization Ideas

### High Priority:
1. **Transposition Tables** - Cache evaluated positions (50-70% speedup)
2. **Better Move Ordering** - Try best moves first (2x speedup)
3. **Null Move Pruning** - Skip some positions (30% speedup)

### Medium Priority:
4. **Killer Move Heuristic** - Remember good moves
5. **Late Move Reductions** - Search late moves shallower
6. **Opening Book** - Pre-computed opening moves

### Low Priority:
7. **Bitboards** - Represent board as 64-bit integers (3-5x speedup, major rewrite)
8. **Endgame Tablebases** - Perfect play in simple endgames
9. **Neural Network Evaluation** - ML-based position evaluation

---

## Code Organization

```
chess.cpp (3200 lines)
├── Lines 1-100:     Includes, enums, forward declarations
├── Lines 101-200:   Piece, ChessMove, Move structures
├── Lines 201-1400:  ChessBoard class (game logic)
├── Lines 1401-1650: Utility classes (PositionHash, MoveOrderer)
├── Lines 1651-2400: Old depth-based AIs (kept for compatibility)
├── Lines 2401-2900: Time-based AIs (MaterialAI, PositionalAI)
└── Lines 2901-3200: Main game loop
```

**Design Philosophy:**
- Single file for simplicity (easy to share/compile)
- Clear separation of game logic (ChessBoard) and AI (ChessAI classes)
- Extensive validation to prevent illegal moves
- Comprehensive undo support for AI search

---

## Compilation & Usage

### Compile:
```bash
make clean && make
```

### Run:
```bash
./chess
```

### Play:
```
Select White Player: 1-4
Select Black Player: 1-4
Enter time limit: 5

Commands:
  e4, Nf3, O-O - Make a move
  save - Export PGN
  undo - Undo last move
  flip - Flip board view
  quit - Exit
```

---

## Summary

This chess engine implements:
- ✅ Complete chess rules (all special moves)
- ✅ Multiple AI strengths
- ✅ Efficient alpha-beta search
- ✅ Time-based play (tournament-style)
- ✅ Advanced evaluation (positional factors)
- ✅ PGN export for analysis
- ✅ Draw detection (all types)

**Key Strengths:**
- Clean, readable code
- Well-tested move generation
- Proper undo/redo for search
- Good performance (depth 5-6 in 5 seconds)

**Next Steps for Users:**
- Play games against the AI
- Export to PGN and analyze
- Try different time limits
- Modify evaluation function for custom play style

Enjoy your chess engine! 🎉
