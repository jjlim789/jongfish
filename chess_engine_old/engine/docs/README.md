# C++ Chess Engine

A modular, fully-featured chess engine written in C++17 with a CLI interface, strong AI, and comprehensive positional evaluation.

> **Created using vibe coding with AI** — combining structured design and AI-driven generation to produce a modular, playable chess engine.

---

## Features

- **Human vs AI** and **Bot vs Bot** modes
- **SAN move input** (`e4`, `Nf3`, `O-O`, `Qxd5+`, etc.)
- **Unicode board display**
- **Undo moves** (undoes both human and AI move)
- **Evaluation display** (`eval` command)
- **PGN export** (`savepgn` command)
- **Perft testing** for move generation verification

### AI Strength
- Iterative deepening alpha-beta search
- Quiescence search (captures only)
- Transposition table with Zobrist hashing
- Principal Variation Search (PVS)
- Killer move heuristic
- History heuristic
- Late Move Reductions (LMR)
- Time-limited search

### Evaluation
- Material values (P=100, N=320, B=330, R=500, Q=900)
- Piece-square tables (middlegame + endgame, tapered)
- Pawn structure: doubled, isolated, backward, passed pawns
- Rooks: open files, semi-open files, 7th rank bonus
- Bishop pair bonus
- Piece mobility
- King safety: pawn shield, attacker proximity, exposure

---

## Build Instructions

### Requirements
- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.14+ **or** GNU Make

### Using Make (simplest)
```bash
make
./chess_engine
```

### Using CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
./chess_engine
```

---

## Usage

### Starting the Engine
```
./chess_engine
```

### Command-line Modes
```bash
# Run perft test
./chess_engine perft 4

# Load a FEN position
./chess_engine fen "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
```

### In-Game Commands
| Command | Description |
|---------|-------------|
| `e4`, `Nf3`, `O-O` | Make a move in SAN notation |
| `undo` | Undo the last move pair (human + AI) |
| `eval` | Display current position evaluation |
| `savepgn <file>` | Export game to PGN file |
| `perft <depth>` | Run perft from current position |
| `quit` | Exit the engine |

---

## Example Sessions

### Human vs AI
```
Select mode: 1
Play as white or black? w
AI thinking time per move (seconds, default 3): 3

  a b c d e f g h
8 ♜ ♞ ♝ ♛ ♚ ♝ ♞ ♜ 8
7 ♟ ♟ ♟ ♟ ♟ ♟ ♟ ♟ 7
...

Your move: e4
Black AI thinking for 3s...
Best move: e5 (nodes: 52843, depth: 6, eval: +15, time: 2.9s)
```

### Bot vs Bot
```
Select mode: 2
AI thinking time per move (seconds, default 3): 2

White AI thinking for 2s...
Best move: e4 (nodes: 48123, depth: 6, eval: +20)
...
```

---

## Folder Structure

```
chess_engine/
├── engine/
│   ├── board/          # Board representation, make/unmake, FEN, Zobrist
│   ├── movegen/        # Legal move generation, perft
│   ├── search/         # Alpha-beta, iterative deepening, TT, heuristics
│   ├── eval/           # Evaluation with PSTs and positional bonuses
│   ├── cli/            # CLI interface, SAN parsing, game loop
│   └── util/           # PGN export
├── tests/              # Perft tests
├── docs/               # Documentation
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

## Running Tests

```bash
make perft_test
./perft_test
```

Expected output:
```
[PASS] Start d1: got 20 expected 20
[PASS] Start d2: got 400 expected 400
[PASS] Start d3: got 8902 expected 8902
[PASS] Pos2 d1: got 48 expected 48
[PASS] Pos2 d2: got 2039 expected 2039
...
7/7 tests passed.
```

---

## License

MIT License — free to use, modify, and distribute.
