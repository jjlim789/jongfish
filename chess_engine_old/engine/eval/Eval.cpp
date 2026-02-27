#include "Eval.h"
#include <array>
#include <cmath>

// Piece-square tables (from White's perspective, a1=0)
// Arranged rank 1..8 bottom to top

static const int PST_PAWN_MG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int PST_PAWN_EG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
     5,  5,  5,  5,  5,  5,  5,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static const int PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

static const int PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

static const int PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

static const int PST_KING_MG[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

static const int PST_KING_EG[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

static int flipSq(int s) {
    // Flip square vertically (mirror rank)
    return (7-(s/8))*8 + (s%8);
}

static int getPST(Piece pt, Color c, int sq, bool endgame) {
    // PST tables are indexed with rank1=row0 (a1=index 0).
    // White: use square directly (a1=0 is White's back rank).
    // Black: flip rank so Black's back rank (a8=56) maps to index 0.
    int s = (c==BLACK) ? flipSq(sq) : sq;
    switch(pt) {
        case PAWN:   return endgame ? PST_PAWN_EG[s] : PST_PAWN_MG[s];
        case KNIGHT: return PST_KNIGHT[s];
        case BISHOP: return PST_BISHOP[s];
        case ROOK:   return PST_ROOK[s];
        case QUEEN:  return PST_QUEEN[s];
        case KING:   return endgame ? PST_KING_EG[s] : PST_KING_MG[s];
        default: return 0;
    }
}

int Eval::materialValue(int pc) {
    if (!pc) return 0;
    static const int vals[] = {0,PAWN_VAL,KNIGHT_VAL,BISHOP_VAL,ROOK_VAL,QUEEN_VAL,KING_VAL,
                                  PAWN_VAL,KNIGHT_VAL,BISHOP_VAL,ROOK_VAL,QUEEN_VAL,KING_VAL};
    return vals[pc];
}

int Eval::gamePhase(const Board& board) {
    // Phase: count minor/major pieces (max=24)
    int phase = 0;
    for (int s=0;s<64;s++) {
        int pc=board.pieceAt(s);
        Piece pt=pieceType(pc);
        if (pt==KNIGHT||pt==BISHOP) phase+=1;
        if (pt==ROOK) phase+=2;
        if (pt==QUEEN) phase+=4;
    }
    return std::min(phase,24);
}

static int evaluatePawnStructure(const Board& board, Color c) {
    Color opp = (c==WHITE)?BLACK:WHITE;
    int score = 0;
    int myPawn = makePiece(c,PAWN);
    int oppPawn = makePiece(opp,PAWN);

    // Count pawns per file
    int myFiles[8]={}, oppFiles[8]={};
    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)==myPawn) myFiles[s%8]++;
        if (board.pieceAt(s)==oppPawn) oppFiles[s%8]++;
    }

    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myPawn) continue;
        int f=s%8, r=s/8;

        // Doubled pawn penalty
        if (myFiles[f]>1) score-=15;

        // Isolated pawn penalty
        bool isolated=true;
        if (f>0&&myFiles[f-1]>0) isolated=false;
        if (f<7&&myFiles[f+1]>0) isolated=false;
        if (isolated) score-=20;

        // Passed pawn bonus
        bool passed=true;
        if (c==WHITE) {
            for (int rr=r+1;rr<=7&&passed;rr++) {
                if (f>0&&board.pieceAt(rr*8+f-1)==oppPawn) passed=false;
                if (board.pieceAt(rr*8+f)==oppPawn) passed=false;
                if (f<7&&board.pieceAt(rr*8+f+1)==oppPawn) passed=false;
            }
            if (passed) score += 20 + r*10;
        } else {
            for (int rr=r-1;rr>=0&&passed;rr--) {
                if (f>0&&board.pieceAt(rr*8+f-1)==oppPawn) passed=false;
                if (board.pieceAt(rr*8+f)==oppPawn) passed=false;
                if (f<7&&board.pieceAt(rr*8+f+1)==oppPawn) passed=false;
            }
            if (passed) score += 20 + (7-r)*10;
        }

        // Backward pawn: pawn that cannot be supported by another friendly pawn
        // and whose advance square is controlled by an enemy pawn.
        // Only penalize if the stop square is attacked by an enemy pawn.
        {
            int stopSq = (c==WHITE) ? s+8 : s-8;
            if (stopSq>=0 && stopSq<64) {
                // Check if stop square is attacked by opponent pawn
                bool stopAttacked = false;
                if (c==WHITE) {
                    if (f>0 && stopSq+8<64 && board.pieceAt(stopSq+8-1)==oppPawn) stopAttacked=true; // no, attacker would be on rank above stop
                    // oppPawn attacks stopSq from stopSq+8±1 (Black pawns attack downward)
                    int atkSq1 = stopSq+8-1, atkSq2 = stopSq+8+1;
                    if (f>0 && atkSq1>=0&&atkSq1<64 && (atkSq1%8)==f-1 && board.pieceAt(atkSq1)==oppPawn) stopAttacked=true;
                    if (f<7 && atkSq2>=0&&atkSq2<64 && (atkSq2%8)==f+1 && board.pieceAt(atkSq2)==oppPawn) stopAttacked=true;
                } else {
                    // White pawns attack upward: they attack stopSq from stopSq-8±1
                    int atkSq1 = stopSq-8-1, atkSq2 = stopSq-8+1;
                    if (f>0 && atkSq1>=0&&atkSq1<64 && (atkSq1%8)==f-1 && board.pieceAt(atkSq1)==oppPawn) stopAttacked=true;
                    if (f<7 && atkSq2>=0&&atkSq2<64 && (atkSq2%8)==f+1 && board.pieceAt(atkSq2)==oppPawn) stopAttacked=true;
                }
                // Check if pawn can be supported by a friendly pawn
                bool canBeSupported = false;
                if (c==WHITE) {
                    // A friendly pawn on f-1 or f+1 could advance to support us
                    if (f>0 && myFiles[f-1]>0) canBeSupported=true;
                    if (f<7 && myFiles[f+1]>0) canBeSupported=true;
                } else {
                    if (f>0 && myFiles[f-1]>0) canBeSupported=true;
                    if (f<7 && myFiles[f+1]>0) canBeSupported=true;
                }
                if (stopAttacked && !canBeSupported) score -= 10;
            }
        }
    }
    return score;
}

static int evaluateRooks(const Board& board, Color c) {
    int score=0;
    int myR=makePiece(c,ROOK);
    int myP=makePiece(c,PAWN);
    int oppP=makePiece((c==WHITE)?BLACK:WHITE,PAWN);
    int seventhRank=(c==WHITE)?6:1;

    // Count my/opp pawns per file
    bool myPawnOnFile[8]={}, oppPawnOnFile[8]={};
    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)==myP) myPawnOnFile[s%8]=true;
        if (board.pieceAt(s)==oppP) oppPawnOnFile[s%8]=true;
    }

    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myR) continue;
        int f=s%8, r=s/8;
        if (!myPawnOnFile[f]) {
            if (!oppPawnOnFile[f]) score+=20; // open file
            else score+=10; // semi-open
        }
        if (r==seventhRank) score+=25; // 7th rank
    }
    return score;
}

static int evaluateKingSafety(const Board& board, Color c, int phase) {
    Color opp=(c==WHITE)?BLACK:WHITE;
    int score=0;
    int myK=makePiece(c,KING);
    Square ks=-1;
    for (int s=0;s<64;s++) if (board.pieceAt(s)==myK) { ks=s; break; }
    if (ks<0) return 0;
    
    int kr=ks/8, kf=ks%8;
    int myP=makePiece(c,PAWN);

    // Pawn shield (only relevant in middlegame)
    if (phase>8) {
        int shieldRank=(c==WHITE)?kr+1:kr-1;
        int shields=0;
        for (int df=-1;df<=1;df++) {
            int sf=kf+df;
            if (sf<0||sf>7) continue;
            if (shieldRank>=0&&shieldRank<8&&board.pieceAt(shieldRank*8+sf)==myP) shields++;
        }
        score += shields*10;
        
        // Penalty for exposed king near center
        if (kf>=2&&kf<=5) score-=20;
    }

    // Count attackers near king (simplified)
    int attackers=0;
    for (int ds=-2;ds<=2;ds++) for (int df=-2;df<=2;df++) {
        int ts=ks+ds*8+df;
        if (ts<0||ts>63) continue;
        int pc=board.pieceAt(ts);
        if (pc && pieceColor(pc)==opp && pieceType(pc)!=PAWN) attackers++;
    }
    score -= attackers*8;

    return score;
}

static int mobility(const Board& board, Color c) {
    // Count squares attacked/reachable by pieces (simplified)
    // Count pseudo-legal moves for bishops, knights, queens, rooks
    int score=0;
    Color opp=(c==WHITE)?BLACK:WHITE;
    
    for (int s=0;s<64;s++) {
        int pc=board.pieceAt(s);
        if (!pc||pieceColor(pc)!=c) continue;
        Piece pt=pieceType(pc);
        if (pt==PAWN||pt==KING) continue;
        
        int moves=0;
        if (pt==KNIGHT) {
            int off[]={-17,-15,-10,-6,6,10,15,17};
            for (int o:off) {
                int t=s+o;
                if (t<0||t>63) continue;
                int df=abs(t%8-s%8);
                if (df!=1&&df!=2) continue;
                int cap=board.pieceAt(t);
                if (!cap||pieceColor(cap)!=c) moves++;
            }
            score+=moves*2;
        } else if (pt==BISHOP||pt==QUEEN) {
            int dirs[]={9,-9,7,-7};
            for (int d:dirs) {
                int t=s;
                while(true) {
                    int nt=t+d;
                    if (nt<0||nt>63) break;
                    if (abs(nt%8-t%8)!=1) break;
                    int cap=board.pieceAt(nt);
                    if (cap&&pieceColor(cap)==c) break;
                    moves++;
                    if (cap) break;
                    t=nt;
                }
            }
            if (pt==BISHOP) score+=moves*2;
        }
        if (pt==ROOK||pt==QUEEN) {
            int dirs[]={1,-1,8,-8};
            for (int d:dirs) {
                int t=s;
                while(true) {
                    int nt=t+d;
                    if (nt<0||nt>63) break;
                    if (abs(d)==1&&nt/8!=s/8) break;
                    int cap=board.pieceAt(nt);
                    if (cap&&pieceColor(cap)==c) break;
                    moves++;
                    if (cap) break;
                    t=nt;
                }
            }
            if (pt==ROOK) score+=moves;
            if (pt==QUEEN) score+=moves;
        }
    }
    return score;
}

int Eval::evaluate(const Board& board) {
    int phase = gamePhase(board);
    bool endgame = (phase < 10);
    float eg = 1.0f - (float)phase/24.0f;

    int score = 0;

    // Material + PST
    for (int s=0;s<64;s++) {
        int pc=board.pieceAt(s);
        if (!pc) continue;
        Color c=pieceColor(pc);
        Piece pt=pieceType(pc);
        int val = materialValue(pc);
        int pst_mg = getPST(pt,c,s,false);
        int pst_eg = getPST(pt,c,s,true);
        int pst = (int)(pst_mg*(1-eg) + pst_eg*eg);
        int total = val + pst;
        if (c==WHITE) score+=total; else score-=total;
    }

    // Pawn structure
    score += evaluatePawnStructure(board, WHITE);
    score -= evaluatePawnStructure(board, BLACK);

    // Rooks
    score += evaluateRooks(board, WHITE);
    score -= evaluateRooks(board, BLACK);

    // Bishop pair bonus
    if (board.countPiece(WHITE,BISHOP)>=2) score+=30;
    if (board.countPiece(BLACK,BISHOP)>=2) score-=30;

    // Mobility
    score += mobility(board, WHITE);
    score -= mobility(board, BLACK);

    // King safety (weighted by phase)
    int wks = evaluateKingSafety(board,WHITE,phase);
    int bks = evaluateKingSafety(board,BLACK,phase);
    score += (int)(wks*(float)phase/24.0f);
    score -= (int)(bks*(float)phase/24.0f);

    return score;
}
