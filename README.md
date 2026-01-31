# Chess CLI Application

A fully interactive command-line chess game written in C++.

## Features

- Full chess board with standard starting position
- Move pieces using standard algebraic notation (e.g., `e4`, `Nf3`, `Bxc4`, `O-O`)
- Complete move validation for all piece types
- **Castling support** (kingside and queenside)
- **Check, checkmate, and stalemate detection**
- Prevents illegal moves that would leave king in check
- **Move history with undo** - take back moves with `u` or `undo`
- Pawn promotion to any piece (Queen, Rook, Bishop, Knight)
- En passant capture support
- Flip the board view with `f` or `flip` command
- Turn-based gameplay (White moves first)

## Compilation

```bash
make
```

Or compile manually:
```bash
g++ -std=c++11 -Wall -Wextra -o chess chess.cpp
```

## Running

```bash
./chess
```

Or:
```bash
make run
```

## How to Play

### Moving Pieces

The game supports standard algebraic notation (SAN) used by chess platforms like Lichess and Chess.com:

**Basic moves:**
- `e4` - Move pawn to e4
- `Nf3` - Move knight to f3
- `Bc4` - Move bishop to c4

**Captures:**
- `Nxf3` - Knight captures on f3
- `exd5` - Pawn on e-file captures on d5
- `Bxc4` - Bishop captures on c4

**Disambiguation (when multiple pieces can move to the same square):**
- `Nbd7` - Knight from b-file to d7
- `R1a3` - Rook from rank 1 to a3
- `Qh4e1` - Queen from h4 to e1

**Pawn promotion:**
- `e8=Q` - Promote pawn to queen (with = sign)
- `e8Q` - Promote pawn to queen (without = sign)
- `e1=N` - Promote pawn to knight
- `d8=R` - Promote pawn to rook
- `e8=B` - Promote pawn to bishop
- If no piece is specified, promotion defaults to queen

**En passant:**
- Automatically tracked and validated
- Use normal capture notation: `exd6` (if white pawn on e5 can capture black pawn that just moved d7-d5)
- Works for both white and black pawns

**Castling:**
- `O-O` or `0-0` - Kingside castling
- `O-O-O` or `0-0-0` - Queenside castling
- Automatically validates castling legality (king/rook not moved, squares not under attack, path clear)

**Long notation (also supported):**
- `e2e4` or `e2-e4` - Move from e2 to e4
- `e2xe4` - Capture from e2 to e4

The game will validate moves according to chess rules and only accept legal moves.

### Commands

- `f` or `flip` - Flip the board view (switches between White and Black perspective)
- `u` or `undo` - Undo the last move (can undo multiple times)
- `q` or `quit` or `exit` - Quit the game

### Board Display

- Uppercase letters = White pieces (P, N, B, R, Q, K)
- Lowercase letters = Black pieces (p, n, b, r, q, k)
- `.` = Empty square

Piece symbols:
- P/p = Pawn
- N/n = Knight
- B/b = Bishop
- R/r = Rook
- Q/q = Queen
- K/k = King

## Example Game

```
8 r n b q k b n r 
7 p p p p p p p p 
6 . . . . . . . . 
5 . . . . . . . . 
4 . . . . . . . . 
3 . . . . . . . . 
2 P P P P P P P P 
1 R N B Q K B N R 
  a b c d e f g h

White to move

Enter move: e4

8 r n b q k b n r 
7 p p p p p p p p 
6 . . . . . . . . 
5 . . . . . . . . 
4 . . . . P . . . 
3 . . . . . . . . 
2 P P P P . P P P 
1 R N B Q K B N R 
  a b c d e f g h

Black to move

Enter move: e5

Enter move: Nf3

Enter move: Nc6

Enter move: Bc4

Enter move: flip

1 R . . K Q B N R 
2 P P . P P P P P 
3 . . N . . . . . 
4 . . . . . B . . 
5 . . . p p . . . 
6 . . . . . n . . 
7 p p p p . p p p 
8 r . b k q b . r 
  h g f e d c b a

Black to move
```

## Game Status Indicators

The game displays the current status after each move:
- **IN CHECK!** - The current player's king is under attack
- **CHECKMATE!** - Game over, the attacking player wins
- **STALEMATE!** - Game over, it's a draw

## Future Enhancements

Feel free to extend this with:
- Threefold repetition and fifty-move rule detection
- Save/load games in PGN format
- Move notation display in algebraic format
- Time controls
- AI opponent
- Opening book
