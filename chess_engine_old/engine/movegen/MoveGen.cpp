#include "MoveGen.h"
#include <iostream>

static void addPawnMoves(const Board& board, std::vector<Move>& moves, bool capturesOnly) {
    Color side = board.sideToMove();
    int dir = (side==WHITE)?8:-8;
    int startRank = (side==WHITE)?1:6;
    int promoRank = (side==WHITE)?7:0;

    int wP = makePiece(WHITE,PAWN), bP = makePiece(BLACK,PAWN);
    int myPawn = (side==WHITE)?wP:bP;

    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myPawn) continue;
        int f=s%8, r=s/8;

        // Captures
        for (int df : {-1,1}) {
            int tf=f+df;
            if (tf<0||tf>7) continue;
            int t=s+dir+df;
            if (t<0||t>63) continue;
            // En passant
            if (t==board.epSquare()) {
                moves.push_back(Move(s,t,FLAG_EP));
                continue;
            }
            int cap = board.pieceAt(t);
            if (cap && pieceColor(cap)!=side) {
                if (t/8==promoRank) {
                    for (int p=0;p<4;p++) moves.push_back(Move(s,t,FLAG_PROMO,p));
                } else {
                    moves.push_back(Move(s,t,FLAG_NORMAL));
                }
            }
        }

        if (capturesOnly) continue;

        // Forward
        int fwd = s+dir;
        if (fwd<0||fwd>63) continue;
        if (board.pieceAt(fwd)==0) {
            if (fwd/8==promoRank) {
                for (int p=0;p<4;p++) moves.push_back(Move(s,fwd,FLAG_PROMO,p));
            } else {
                moves.push_back(Move(s,fwd,FLAG_NORMAL));
                // Double push
                if (r==startRank) {
                    int dbl=fwd+dir;
                    if (board.pieceAt(dbl)==0) moves.push_back(Move(s,dbl,FLAG_NORMAL));
                }
            }
        }
    }
}

static void addKnightMoves(const Board& board, std::vector<Move>& moves, bool capturesOnly) {
    Color side = board.sideToMove();
    int myN = makePiece(side,KNIGHT);
    int offsets[] = {-17,-15,-10,-6,6,10,15,17};
    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myN) continue;
        int sf=s%8,sr=s/8;
        for (int o : offsets) {
            int t=s+o;
            if (t<0||t>63) continue;
            int df=abs(t%8-sf),dr=abs(t/8-sr);
            if (!((df==1&&dr==2)||(df==2&&dr==1))) continue;
            int cap=board.pieceAt(t);
            if (cap && pieceColor(cap)==side) continue; // own piece
            if (capturesOnly && !cap) continue;
            moves.push_back(Move(s,t,FLAG_NORMAL));
        }
    }
}

static void addSlidingMoves(const Board& board, std::vector<Move>& moves, Piece ptype, bool capturesOnly) {
    Color side = board.sideToMove();
    int myPc = makePiece(side,ptype);
    
    int rookDirs[] = {1,-1,8,-8,0};
    int bishDirs[] = {9,-9,7,-7,0};
    int* dirs = (ptype==ROOK)?rookDirs:(ptype==BISHOP)?bishDirs:nullptr;
    int allDirs[] = {1,-1,8,-8,9,-9,7,-7,0};
    if (ptype==QUEEN) dirs = allDirs;

    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myPc) continue;
        for (int di=0;dirs[di]!=0;di++) {
            int d=dirs[di];
            int t=s+d;
            while (t>=0&&t<64) {
                // Check diagonal overflow
                int df=abs(t%8-s%8), dr=abs(t/8-s/8);
                int steps=std::max(df,dr);
                if (df>steps||dr>steps) break; // impossible actually
                // Simpler: check file wrap
                if ((d==1||d==-1||d==9||d==-9||d==7||d==-7) && df!=dr && d!=1 && d!=-1) {
                    // diagonal: check consistent
                }
                // Prevent rank wrapping for horizontal dirs
                if ((d==1||d==-1) && (t/8)!=(s/8)) break;
                // Prevent file wrapping for diagonal
                if ((d==9||d==7) && (t%8)<=( (s%8))) { if(d==9&&t%8<=s%8) {} }
                // Cleaner approach: just track file distance
                {
                    // recompute per step
                    int sf=s%8;
                    // Walk step by step, check file continuity
                }

                int cap=board.pieceAt(t);
                if (cap && pieceColor(cap)==side) break;
                if (!capturesOnly || cap) moves.push_back(Move(s,t,FLAG_NORMAL));
                if (cap) break;
                t+=d;
                // Wrap check
                if (t<0||t>63) break;
                // Horizontal wrap
                if ((d==1) && t%8==0) break;
                if ((d==-1) && t%8==7) break;
                // Diagonal wrap
                if ((d==9||d==-7) && t%8==0) break;
                if ((d==7||d==-9) && t%8==7) break;
            }
        }
    }
}

// Better sliding move generator
static void addSliding(const Board& board, std::vector<Move>& moves, bool capturesOnly) {
    Color side = board.sideToMove();
    Color opp = (side==WHITE)?BLACK:WHITE;

    for (int s=0;s<64;s++) {
        int pc = board.pieceAt(s);
        if (!pc || pieceColor(pc)!=side) continue;
        Piece pt = pieceType(pc);
        if (pt!=ROOK&&pt!=BISHOP&&pt!=QUEEN) continue;

        bool isRook = (pt==ROOK||pt==QUEEN);
        bool isBish = (pt==BISHOP||pt==QUEEN);

        if (isRook) {
            // Horizontal/vertical
            int rdirs[] = {1,-1,8,-8};
            for (int d : rdirs) {
                int t=s;
                while (true) {
                    int nt=t+d;
                    if (nt<0||nt>63) break;
                    if (d==1&&nt%8==0) break;
                    if (d==-1&&nt%8==7) break;
                    // Check same rank for horiz
                    if (abs(d)==1 && nt/8!=s/8) break;
                    int cap=board.pieceAt(nt);
                    if (cap && pieceColor(cap)==side) break;
                    if (!capturesOnly||cap) moves.push_back(Move(s,nt,FLAG_NORMAL));
                    if (cap) break;
                    t=nt;
                }
            }
        }
        if (isBish) {
            int bdirs[] = {9,-9,7,-7};
            for (int d : bdirs) {
                int t=s;
                while (true) {
                    int nt=t+d;
                    if (nt<0||nt>63) break;
                    int df=abs(nt%8-t%8);
                    if (df!=1) break; // diagonal must step 1 file
                    int cap=board.pieceAt(nt);
                    if (cap && pieceColor(cap)==side) break;
                    if (!capturesOnly||cap) moves.push_back(Move(s,nt,FLAG_NORMAL));
                    if (cap) break;
                    t=nt;
                }
            }
        }
    }
}

static void addKingMoves(const Board& board, std::vector<Move>& moves, bool capturesOnly) {
    Color side = board.sideToMove();
    int myK = makePiece(side,KING);
    int koffsets[] = {-9,-8,-7,-1,1,7,8,9};
    
    for (int s=0;s<64;s++) {
        if (board.pieceAt(s)!=myK) continue;
        int sf=s%8,sr=s/8;
        for (int o : koffsets) {
            int t=s+o;
            if (t<0||t>63) continue;
            if (abs(t%8-sf)>1||abs(t/8-sr)>1) continue;
            int cap=board.pieceAt(t);
            if (cap && pieceColor(cap)==side) continue;
            if (capturesOnly && !cap) continue;
            moves.push_back(Move(s,t,FLAG_NORMAL));
        }

        if (capturesOnly) continue;

        // Castling
        Color opp=(side==WHITE)?BLACK:WHITE;
        int cr=board.castlingRights();
        if (side==WHITE) {
            // Kingside
            if ((cr&1) && board.pieceAt(5)==0 && board.pieceAt(6)==0 &&
                !board.isSquareAttacked(4,opp)&&!board.isSquareAttacked(5,opp)&&!board.isSquareAttacked(6,opp)) {
                moves.push_back(Move(4,6,FLAG_CASTLE));
            }
            // Queenside
            if ((cr&2) && board.pieceAt(3)==0 && board.pieceAt(2)==0 && board.pieceAt(1)==0 &&
                !board.isSquareAttacked(4,opp)&&!board.isSquareAttacked(3,opp)&&!board.isSquareAttacked(2,opp)) {
                moves.push_back(Move(4,2,FLAG_CASTLE));
            }
        } else {
            if ((cr&4) && board.pieceAt(61)==0 && board.pieceAt(62)==0 &&
                !board.isSquareAttacked(60,opp)&&!board.isSquareAttacked(61,opp)&&!board.isSquareAttacked(62,opp)) {
                moves.push_back(Move(60,62,FLAG_CASTLE));
            }
            if ((cr&8) && board.pieceAt(59)==0 && board.pieceAt(58)==0 && board.pieceAt(57)==0 &&
                !board.isSquareAttacked(60,opp)&&!board.isSquareAttacked(59,opp)&&!board.isSquareAttacked(58,opp)) {
                moves.push_back(Move(60,58,FLAG_CASTLE));
            }
        }
    }
}

std::vector<Move> MoveGen::generateMoves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(64);
    addPawnMoves(board,moves,false);
    addKnightMoves(board,moves,false);
    addSliding(board,moves,false);
    addKingMoves(board,moves,false);
    return moves;
}

std::vector<Move> MoveGen::generateCaptures(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(16);
    addPawnMoves(board,moves,true);
    addKnightMoves(board,moves,true);
    addSliding(board,moves,true);
    addKingMoves(board,moves,true);
    return moves;
}

std::vector<Move> MoveGen::generateLegalMoves(Board& board) {
    auto pseudo = generateMoves(board);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());
    for (auto& m : pseudo) {
        if (board.makeMove(m)) {
            legal.push_back(m);
            board.unmakeMove();
        }
    }
    return legal;
}

uint64_t MoveGen::perft(Board& board, int depth) {
    if (depth==0) return 1;
    auto moves = generateMoves(board);
    uint64_t nodes=0;
    for (auto& m : moves) {
        if (board.makeMove(m)) {
            nodes += perft(board,depth-1);
            board.unmakeMove();
        }
    }
    return nodes;
}
