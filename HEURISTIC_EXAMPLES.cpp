// Example: Custom Heuristic Implementation
// This file shows how to modify the heuristic() function in chess.cpp
// to implement your own chess evaluation function

/*
Replace the heuristic() method in the ChessBoard class with your custom implementation:

double ChessBoard::heuristic(Color perspective) const {
    // Check for terminal positions first
    if (isCheckmate(perspective)) {
        return -numeric_limits<double>::infinity();
    }
    
    Color opponent = (perspective == WHITE) ? BLACK : WHITE;
    if (isCheckmate(opponent)) {
        return numeric_limits<double>::infinity();
    }
    
    double score = 0.0;
    
    // 1. MATERIAL EVALUATION
    // Basic piece values
    const double PAWN_VALUE = 1.0;
    const double KNIGHT_VALUE = 3.0;
    const double BISHOP_VALUE = 3.2;  // Slightly favor bishops
    const double ROOK_VALUE = 5.0;
    const double QUEEN_VALUE = 9.0;
    
    // 2. POSITIONAL EVALUATION
    // Piece-square tables (bonus for good square placement)
    
    // Pawn position bonuses (encourage center pawns and advancement)
    const double pawnTable[8][8] = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0},
        {1.0, 1.0, 2.0, 3.0, 3.0, 2.0, 1.0, 1.0},
        {0.5, 0.5, 1.0, 2.5, 2.5, 1.0, 0.5, 0.5},
        {0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 0.0},
        {0.5,-0.5,-1.0, 0.0, 0.0,-1.0,-0.5, 0.5},
        {0.5, 1.0, 1.0,-2.0,-2.0, 1.0, 1.0, 0.5},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
    };
    
    // Knight position bonuses (encourage center control)
    const double knightTable[8][8] = {
        {-5.0,-4.0,-3.0,-3.0,-3.0,-3.0,-4.0,-5.0},
        {-4.0,-2.0, 0.0, 0.0, 0.0, 0.0,-2.0,-4.0},
        {-3.0, 0.0, 1.0, 1.5, 1.5, 1.0, 0.0,-3.0},
        {-3.0, 0.5, 1.5, 2.0, 2.0, 1.5, 0.5,-3.0},
        {-3.0, 0.0, 1.5, 2.0, 2.0, 1.5, 0.0,-3.0},
        {-3.0, 0.5, 1.0, 1.5, 1.5, 1.0, 0.5,-3.0},
        {-4.0,-2.0, 0.0, 0.5, 0.5, 0.0,-2.0,-4.0},
        {-5.0,-4.0,-3.0,-3.0,-3.0,-3.0,-4.0,-5.0}
    };
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = board[row][col];
            if (piece.type == EMPTY) continue;
            
            double pieceValue = 0.0;
            double positionBonus = 0.0;
            
            switch(piece.type) {
                case PAWN:
                    pieceValue = PAWN_VALUE;
                    // Use table for white, flip for black
                    positionBonus = (piece.color == WHITE) ? 
                        pawnTable[row][col] / 10.0 : 
                        pawnTable[7-row][col] / 10.0;
                    break;
                    
                case KNIGHT:
                    pieceValue = KNIGHT_VALUE;
                    positionBonus = knightTable[row][col] / 10.0;
                    break;
                    
                case BISHOP:
                    pieceValue = BISHOP_VALUE;
                    // Bonus for bishop pair (implement if needed)
                    break;
                    
                case ROOK:
                    pieceValue = ROOK_VALUE;
                    // Bonus for rooks on open files (implement if needed)
                    break;
                    
                case QUEEN:
                    pieceValue = QUEEN_VALUE;
                    break;
                    
                case KING:
                    pieceValue = 0.0;
                    // King safety evaluation (implement if needed)
                    break;
                    
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
    
    // 3. MOBILITY (number of legal moves)
    // More moves = better position
    int myMoves = generateLegalMoves(perspective).size();
    int opponentMoves = generateLegalMoves(opponent).size();
    score += (myMoves - opponentMoves) * 0.1;
    
    // 4. KING SAFETY
    // Penalize exposed king in middlegame
    auto [myKingRow, myKingCol] = findKing(perspective);
    auto [oppKingRow, oppKingCol] = findKing(opponent);
    
    // Count attackers near king
    // (implement detailed king safety if needed)
    
    // 5. PAWN STRUCTURE
    // Penalize doubled pawns, isolated pawns
    // Bonus for passed pawns
    // (implement if needed)
    
    // 6. CONTROL OF CENTER
    // Bonus for controlling e4, d4, e5, d5
    // (implement if needed)
    
    return score;
}
*/

// SIMPLER EXAMPLE: Position-aware heuristic
/*
double ChessBoard::heuristic(Color perspective) const {
    if (isCheckmate(perspective)) return -numeric_limits<double>::infinity();
    if (isCheckmate((perspective == WHITE) ? BLACK : WHITE)) 
        return numeric_limits<double>::infinity();
    
    double score = 0.0;
    
    // Material + simple positional bonuses
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = board[row][col];
            if (p.type == EMPTY) continue;
            
            double value = 0.0;
            switch(p.type) {
                case PAWN: value = 1.0; break;
                case KNIGHT: value = 3.0; break;
                case BISHOP: value = 3.2; break;
                case ROOK: value = 5.0; break;
                case QUEEN: value = 9.0; break;
                default: value = 0.0;
            }
            
            // Bonus for center control (e4, d4, e5, d5)
            if ((row == 3 || row == 4) && (col == 3 || col == 4)) {
                value += 0.5;
            }
            
            if (p.color == perspective) {
                score += value;
            } else {
                score -= value;
            }
        }
    }
    
    return score;
}
*/

// ADVANCED EXAMPLE: With alpha-beta enhancements
/*
You can also add:
- Quiescence search (search captures to avoid horizon effect)
- Iterative deepening
- Transposition tables (hash positions to avoid re-evaluation)
- Move ordering (check promising moves first for better pruning)
- Opening book
- Endgame tablebases

Example move ordering in alpha-beta:
1. Previous best move (from iterative deepening)
2. Captures (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
3. Killer moves (moves that caused cutoffs at same depth)
4. Other moves
*/
