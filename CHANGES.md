# Chess Engine - Change Log

## Latest Update (2026-02-02) - Draw Detection & Time Controls

### New Features

#### 1. **Threefold Repetition Detection** ✅
- Automatically detects when the same position occurs three times
- Declares draw immediately when detected
- Position tracking uses board state + turn
- Properly handles position history in undo

#### 2. **Fifty-Move Rule Detection** ✅  
- Tracks half-move clock (resets on pawn moves or captures)
- Declares draw when 50 full moves (100 half-moves) pass without progress
- Restores clock correctly on undo
- Stored in move history for proper undo support

#### 3. **Iterative Deepening with Time Limits** ✅
- New AI type: **Iterative Deepening AI**
- Uses time limits instead of fixed depth (1-60 seconds configurable)
- Progressively searches depth 1, 2, 3... until time expires
- Shows progress during search:
  ```
  Depth 1 completed: eval=0.35 (0.02s)
  Depth 2 completed: eval=0.50 (0.08s)
  Depth 3 completed: eval=0.45 (0.31s)
  Depth 4 completed: eval=0.60 (1.24s)
  Depth 5 incomplete (time limit)
  ```
- **More realistic gameplay** - adapts depth to position complexity
- Uses previous depth results for better move ordering
- Always returns best move found before time expired

**Benefits of Iterative Deepening:**
- Predictable time per move (important for timed games)
- Complex positions get appropriate depth automatically
- Simple positions search deeper with remaining time
- Better move ordering at each depth
- Can be interrupted anytime with valid move

#### 4. **Updated Draw Detection in Game Loop** ✅
- Checks for draws after every move
- Displays appropriate message:
  - "Draw by threefold repetition!"
  - "Draw by fifty-move rule!"
  - "Stalemate!"
- Sets game result to "1/2-1/2"
- Offers to save drawn games

### Technical Implementation

**Position Hashing:**
```cpp
class PositionHash {
    static string hashPosition(const ChessBoard& board);
};

map<string, int> positionHistory;  // Position -> occurrence count
```

**Move Tracking:**
```cpp
struct Move {
    int halfMoveClock;  // Stored for undo support
    // ... other fields
};
```

**Time-Based Search:**
```cpp
class IterativeDeepeningAI {
    double timeLimit;
    clock_t searchStartTime;
    bool isTimeUp();
    // Searches progressively deeper until time expires
};
```

## Previous Update (2026-02-02) - SAN & Evaluation

### Major Improvements

#### 1. **Standard Algebraic Notation (SAN)** ✅
- **Problem**: PGN and CLI output showed moves as `e2e4` instead of proper chess notation
- **Solution**: Implemented full SAN conversion
  - Pawn moves: `e4`, `e5`
  - Piece moves: `Nf3`, `Bc4`, `Qh5`
  - Captures: `Nxf3`, `exd5`
  - Castling: `O-O`, `O-O-O`
  - Promotions: `e8=Q`
  - Disambiguation: `Nbd7`, `R1a3` (when multiple pieces can move to same square)
- **Files modified**: `chess.cpp` - Added `ChessMove::toSAN()` method, updated move storage and PGN export

#### 2. **Objective Evaluation Display** ✅
- **Problem**: Evaluation was subjective (negative when AI was losing)
- **Solution**: All evaluations now from white's perspective
  - Positive = White winning
  - Negative = Black winning
  - Consistent in both CLI output and PGN files
- **Example**: 
  - Before: Black AI shows `-2.05` when winning
  - After: Black AI shows `+2.05` when winning (white perspective)

#### 3. **Fixed Human Undo** ✅
- **Problem**: Undo only took back AI move, AI would repeat same move
- **Solution**: `undo` command now undoes both moves (human + AI)
  - In Human vs AI: Undoes last 2 moves
  - In Human vs Human: Undoes last 1 move
  - Properly restores board state

#### 4. **Code Cleanup** ✅
Removed unused/unnecessary code:
- ❌ Deleted `convertPositionToFEN()` - no longer used
- ❌ Deleted user-implementable `heuristic()` function
- ❌ Removed `const_cast` hacks in board evaluation
- ✅ Cleaner, more maintainable codebase

## Debugging Guide for AI Strength

### Why is my AI weak?

The AI uses several components that can be debugged independently:

#### 1. **Search Depth**
- **What it does**: How many moves ahead the AI looks
- **Debug**: Increase `maxDepth` in AI constructor
- **Testing**:
  ```bash
  # Run with different depths
  Depth 3: ~1-3 seconds per move
  Depth 4: ~5-15 seconds per move
  Depth 5: ~30-90 seconds per move
  ```
- **Expected**: Each extra depth should roughly 10x stronger but 10x slower

#### 2. **Position Evaluation** 
The `positionalEval()` function scores positions. Current weights:

```cpp
PAWN_VALUE = 1.0
KNIGHT_VALUE = 3.0
BISHOP_VALUE = 3.2
ROOK_VALUE = 5.0
QUEEN_VALUE = 9.0
```

**Debug tactics**:
- Add print statements in `positionalEval()` to see scores
- Test specific positions:
  ```cpp
  cout << "Material score: " << materialScore << endl;
  cout << "Position bonus: " << positionBonus << endl;
  cout << "Mobility: " << mobilityScore << endl;
  ```

#### 3. **Move Ordering**
Good move ordering = better pruning = deeper search

**Test move ordering quality**:
```cpp
// In MoveOrderer::orderMoves(), add:
for (auto& [score, move] : scoredMoves) {
    cout << "Move " << move.toSAN(board) << " score: " << score << endl;
}
```

**What to look for**:
- Captures should score highest (1000+)
- Checks/attacks should score high
- Quiet moves should score lowest

#### 4. **Pruning Effectiveness**
Count nodes searched vs. nodes theoretically possible:

```cpp
// Add to alphaBeta():
static int nodesSearched = 0;
nodesSearched++;

// At end of search:
cout << "Nodes searched: " << nodesSearched << endl;
```

**Expected ratios**:
- No pruning: ~35^depth nodes
- Good alpha-beta: ~6-10^depth nodes
- If ratio is bad, move ordering needs work

#### 5. **Tactical Awareness**
Test if AI sees tactics:

**Positions to test**:
1. **Fork**: Can AI fork king and rook?
2. **Pin**: Does AI exploit pinned pieces?
3. **Skewer**: Can AI see skewers?
4. **Discovery**: Does AI find discovered attacks?

**How to test**:
- Set up tactical position
- Run AI at depth 4-5
- Check if AI finds the tactic
- If not, increase depth or improve evaluation

#### 6. **Opening Principles**
Current penalties:
- Unsupported central knight: -1.5
- Trapped knight (≤2 escapes): -2.0
- Early queen: -1.0
- Undeveloped pieces: -0.15 each

**Tune these**:
```cpp
// In positionalEval(), adjust:
if (!supported) {
    positionBonus -= 1.5;  // Try different values
}
```

### Performance Profiling

Add timing to see where time is spent:

```cpp
#include <chrono>

// In alphaBeta():
auto start = chrono::high_resolution_clock::now();
// ... search code ...
auto end = chrono::high_resolution_clock::now();
auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
cout << "Search took: " << duration.count() << "ms" << endl;
```

### Common Weaknesses & Fixes

| Weakness | Likely Cause | Fix |
|----------|--------------|-----|
| Hangs pieces | Depth too low | Increase to 4-5 |
| Passive play | Mobility weight too low | Increase mobility bonus |
| Poor tactics | No quiescence search | Already implemented |
| Slow search | Bad move ordering | Improve ordering heuristic |
| Blunders | Evaluation bug | Add debug prints |
| Bad openings | Opening principles weak | Adjust penalties |

### Testing Specific Issues

**Test piece values**:
```cpp
// Create position with material imbalance
// Run AI, check if it values pieces correctly
double score = chess.evaluatePosition(WHITE);
cout << "Position score: " << score << endl;
```

**Test horizon effect**:
```cpp
// Set up position where capture leads to loss
// Without quiescence: AI takes
// With quiescence: AI avoids
// PositionalAI has quiescence, MaterialisticAI doesn't
```

### Quick Strength Test

Run this game to test tactical vision:
```
1. e4 e5 2. Nf3 Nc6 3. Bc4 Nf6 4. Ng5 d5
```

**What should AI do**?
- Depth 3: May not see the threat
- Depth 4: Should see and defend
- Depth 5: Should find best defense

If AI hangs the f7 pawn, evaluation or search is weak.

## Technical Debt Addressed

1. ✅ Removed `const_cast` hacks
2. ✅ Deleted unused FEN conversion
3. ✅ Removed placeholder heuristic function
4. ✅ Cleaned up move history management
5. ✅ Standardized evaluation perspective

## Remaining Optimization Opportunities

### High Impact (Recommended)

1. **Transposition Table** (50-80% speedup)
   - Hash board positions
   - Store evaluations
   - Avoid re-searching same position
   - Implementation: ~200 lines

2. **Iterative Deepening** (Better move ordering)
   - Search depth 1, then 2, then 3...
   - Use results to order moves
   - Minimal overhead, major benefit
   - Implementation: ~50 lines

3. **Move Generation Optimization**
   - Current: Generate all moves, then filter illegal
   - Better: Only generate legal moves
   - Speedup: 2-3x
   - Implementation: ~100 lines rewrite

### Medium Impact

4. **Bitboards** (Major rewrite, 3-5x speedup)
   - Replace `Piece board[8][8]` with 64-bit integers
   - Extremely fast move generation
   - Implementation: ~500 lines, high complexity

5. **Null Move Pruning** (Search depth +1-2)
   - Give opponent free move
   - If still winning, prune branch
   - Implementation: ~30 lines

6. **Late Move Reductions** (Search depth +1)
   - Search late moves at reduced depth
   - Re-search if they look good
   - Implementation: ~40 lines

### Low Impact (Not recommended now)

7. **Opening Book** (Early game only)
8. **Endgame Tablebases** (Endgame only)
9. **Multi-threading** (Complex, marginal gains)

## File Structure

- `chess.cpp` - Main game engine (~2200 lines)
- `Makefile` - Build configuration
- `README.md` - User documentation
- `PERFORMANCE_ANALYSIS.md` - Performance guide
- `CHANGES.md` - This file

## Attribution

**This entire project is generated by Claude Sonnet 4.5 with prompts by me**
