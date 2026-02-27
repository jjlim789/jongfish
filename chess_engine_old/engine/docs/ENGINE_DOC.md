# Chess Engine — Technical Documentation

> Created using **vibe coding with AI**, combining structured design principles and AI-driven code generation to produce a modular, playable chess engine.

---

## Architecture Overview

The engine is organized into six decoupled modules:

```
board/    ←── movegen/
  ↓             ↓
eval/  ←─── search/
  ↓
cli/ ←── util/
```

Each module has minimal dependencies on others and can be tested independently.

---

## Module: `board/`

### Design

Uses a simple **8x8 array** (64-element `int` array) for board representation, trading some speed for clarity. Each cell holds an integer:

- `0` = empty
- `1–6` = White Pawn, Knight, Bishop, Rook, Queen, King
- `7–12` = Black Pawn, Knight, Bishop, Rook, Queen, King

This makes piece iteration straightforward and avoids complex bitboard operations while still achieving reasonable performance.

### Key Components

**`BoardState`** — A snapshot of the full game state:
- `squares[64]` — piece placement
- `castling` — 4-bit castling rights (K=bit0, Q=bit1, k=bit2, q=bit3)
- `epSquare` — en passant target square (or NO_SQ)
- `halfmove`, `fullmove` — clock counters
- `zobrist` — incremental Zobrist hash
- `capturedPiece`, `prevCastling`, etc. — for `unmakeMove()`

**`make_move`** — Applies a move to the board state:
1. Saves current `BoardState` to `stateHistory` stack
2. Updates Zobrist hash (XOR in/out pieces, ep, castling)
3. Handles special moves: castling (also moves rook), en passant (removes captured pawn), promotion (replaces pawn with new piece)
4. Updates castling rights (remove if king/rook moves from home square)
5. Switches side to move
6. Validates legality: if the moving side's king is in check, **reverts** and returns `false`

**`unmake_move`** — Restores the previous `BoardState` from the stack in O(1).

### Zobrist Hashing

Zobrist keys are precomputed using a deterministic Mersenne Twister seeded with a fixed value, ensuring reproducibility:
- `zKeys[13][64]` — one key per (piece, square) pair
- `zSide` — flipped when side changes
- `zCastle[16]` — one per castling rights bitmask
- `zEP[8]` — one per en passant file

The hash is updated incrementally during `makeMove` and restored by `unmakeMove`.

---

## Module: `movegen/`

### Design

**Pseudo-legal** generation followed by legality check via `makeMove`:
1. Generate all candidate moves for the side to move
2. For each, call `board.makeMove(m)` — which checks if the king is left in check
3. If `makeMove` returns `false`, the move is illegal

This simplifies the generator at a slight performance cost, acceptable for a toy engine.

### Move Encoding

Moves are packed into a `uint16_t`:
```
bits  0–5:  from square (0–63)
bits  6–11: to square (0–63)
bits 12–13: flags (0=normal, 1=castle, 2=en passant, 3=promotion)
bits 14–15: promotion piece (0=N, 1=B, 2=R, 3=Q)
```

### Piece Generators

- **Pawns**: Forward pushes (single/double), diagonal captures, en passant, promotions (generates all 4 promotion moves)
- **Knights**: 8 offsets, file-distance check to prevent rank wrap
- **Sliding pieces (Bishop/Rook/Queen)**: Ray traversal with wrap prevention:
  - Horizontal: break if new square is on a different rank
  - Diagonal: break if `abs(file_change) != 1`
- **King**: 8 adjacent squares + castling (checks intermediate square safety)

### Perft Testing

`MoveGen::perft(board, depth)` recursively counts leaf nodes to validate move generation accuracy. Results verified against known values from the Chess Programming Wiki.

---

## Module: `eval/`

### Tapered Evaluation

The engine uses **tapered evaluation** blending middlegame (MG) and endgame (EG) scores based on the game phase:

```
phase = min(24, sum of: N/B=1, R=2, Q=4)
eg_weight = 1.0 - phase/24.0

score = score_mg * (1 - eg_weight) + score_eg * eg_weight
```

At full material (phase=24): pure middlegame PSTs used.
With no material (phase=0): pure endgame PSTs used.

### Piece-Square Tables

Tables encode positional preferences for each piece type:

- **Pawns (MG)**: Reward central advancement, penalize edge pawns
- **Pawns (EG)**: Strong reward for advancement (passed pawn race)
- **Knights**: Strong centralization bonus, edge penalty
- **Bishops**: Diagonal control, edge penalty
- **Rooks**: Neutral (open file/7th rank handled separately)
- **Queen**: Slight center preference
- **King (MG)**: Strongly prefers castled position, penalizes center
- **King (EG)**: Prefers center (active king)

Black's PSTs are mirrored vertically (flip rank index).

### Pawn Structure

Per-pawn analysis tracking file occupancy:

| Feature | Bonus/Penalty |
|---------|--------------|
| Doubled pawn | -15 per extra pawn on file |
| Isolated pawn | -20 (no friendly pawns on adjacent files) |
| Backward pawn | -10 (can't advance, not supported) |
| Passed pawn | +20 + rank×10 (bigger as it advances) |

### Rook Bonuses

- **Open file** (no pawns of either color): +20
- **Semi-open file** (only opponent pawns): +10
- **7th rank**: +25 (threatens back rank, pins king)

### Bishop Pair

+30 bonus when a side has two or more bishops. Bishops complement each other covering both colored squares.

### Mobility

Counts reachable squares for non-pawn, non-king pieces:
- Knight: +2 per reachable square
- Bishop: +2 per diagonal square
- Rook: +1 per file/rank square
- Queen: +1 per reachable square

Mobility encourages active pieces and penalizes cramped positions.

### King Safety

Applied with weight proportional to game phase (less relevant in endgame):
- **Pawn shield**: +10 per pawn adjacent to king (in front)
- **Exposed king**: -20 if king is on central files (c-f)
- **Attacker proximity**: -8 per enemy non-pawn piece within 2 squares

---

## Module: `search/`

### Iterative Deepening

The engine searches depth 1, 2, 3, ... until time expires. At each depth:
1. Run alpha-beta from depth
2. If time expires mid-search, restore best move from completed depth
3. Return best move found at highest completed depth

This ensures a reasonable move is always available when time expires.

### Alpha-Beta with PVS

The main search uses **Principal Variation Search**:
- First move at each node: full `[alpha, beta]` window
- Subsequent moves: narrow `[alpha, alpha+1]` window
- If the narrow search beats alpha: re-search with full window

This exploits the observation that most moves won't improve alpha, making narrow searches cheap.

### Transposition Table

A hash map indexed by `zobrist % TT_SIZE` (1M entries):

Each entry stores:
- `key` — full Zobrist hash (for collision detection)
- `depth` — search depth of stored result
- `score` — evaluation
- `best` — best move from this position
- `flag` — EXACT / LOWER_BOUND / UPPER_BOUND

On hit: use stored score if depth ≥ current depth. Otherwise use `best` for move ordering.

### Move Ordering

Moves are scored and sorted before the search loop:

| Priority | Source |
|---------|--------|
| 100000 | TT best move |
| 10000+ | Captures (MVV-LVA: victim_value×10 - attacker_value) |
| 9000 | En passant |
| 8000+ | Promotions |
| 7000 | Killer move slot 1 |
| 6900 | Killer move slot 2 |
| variable | History score |

### Killer Moves

Two killer slots per ply store quiet moves that caused beta cutoffs. These moves are tried early in other positions at the same ply, as they often cause cutoffs there too.

### History Heuristic

A 64×64 table `history[from][to]` accumulates `depth²` for quiet moves causing beta cutoffs. Captures are excluded as they're already well-ordered by MVV-LVA.

### Late Move Reductions (LMR)

After the first 4 moves at a node (depth ≥ 3, non-capture, non-promotion, not in check):
- Search at reduced depth: `depth - 1 - R` where R ∈ {1, 2, 3}
- R increases with move count and remaining depth
- If reduced search beats alpha: re-search at full depth

LMR dramatically reduces the search tree since most late moves are poor.

### Quiescence Search

At depth=0, instead of returning a static evaluation, the engine continues searching **captures only** until a quiet position is reached. This prevents the horizon effect (e.g., missing that a queen just captured a pawn but will be recaptured).

---

## Module: `cli/`

### SAN Parsing (`util/PGN.cpp`)

SAN (Standard Algebraic Notation) is parsed by:
1. Detecting castling: `O-O`, `O-O-O`
2. Stripping check/mate symbols (`+`, `#`)
3. Parsing promotion suffix (`=Q`, `=R`, etc.)
4. Extracting piece type (uppercase letter), disambiguation (file/rank), capture (`x`), destination square
5. Matching against all legal moves

SAN generation reverses this process, with disambiguation added when multiple pieces of the same type can reach the destination square.

### Game Flow

```
selectMode() → gameLoop() → [humanTurn() | aiTurn()] → printStatus()
                          ↑___________________________________|
                          (loop until game over)
```

Special commands are handled inline during `humanTurn()`.

---

## Module: `util/`

### PGN Export

Generates standard PGN format with:
- Seven tag roster (Event, Site, Date, Round, White, Black, Result)
- Move text in SAN notation with move numbers
- Result string (1-0, 0-1, 1/2-1/2, *)

---

## Performance Notes

On modern hardware (3GHz CPU), the engine typically achieves:
- ~500k–2M nodes/second
- Depth 6–8 in 3 seconds opening/middlegame
- Deeper in simplified endgames

The array-based board is simpler than bitboards but slower for bulk piece enumeration. Future optimizations could include bitboard representation or more aggressive pruning.

---

## Known Limitations

- No opening book
- No endgame tablebases
- Simplified null move pruning (disabled due to side-toggle complexity)
- No aspiration windows in iterative deepening
- Material draw detection covers only basic cases (K vs K, K+B vs K, K+N vs K)

---

## Design Principles

1. **Correctness first** — Verified by perft tests against known values
2. **Separation of concerns** — Each module is independently testable
3. **Incremental hashing** — Zobrist updated in O(1) during make/unmake
4. **Clean undo** — Full state snapshot makes unmakeMove trivial and bug-free
5. **Time safety** — Search checks `timeUp()` frequently and always returns a legal move
