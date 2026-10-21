# chess-v2
UCI-compatible chess engine written in C++17/20, featuring bitboard-based
move generation, PVS search and a hand-crafted evaluation function.

The engine runs as a standalone executable and communicates through the
UCI protocol. A separate SFML-based GUI is available in [Chess_SFML_GUI](https://github.com/DTB173/Chess_SFML_GUI).

Estimated playing strength: ~2300 Elo (self-measured; methodology below).

## Results

| What | Result | How it was measured |
|---|---|---|
| Move generation speed | ~140M nodes/s (130-150M between runs) | `perft` mode, release build gcc 16.2.0, cpu: i5-13400f |
| Move generation correctness | passes 6-position perft suite | known node counts from the standard perft positions (run `testsuite` mode) |
| Playing strength | ~2300 Elo (estimate) | 1000 games: 500 vs Stockfish limited to Elo 2000, 500 vs Stockfish at 2400; Elo derived from win rates. |

## Features
 
**Board and move generation**
- Hybrid state: bitboards plus a mailbox array for fast piece lookup
- Magic bitboards for sliding piece attacks; most attack tables are computed at compile time
- Generates pseudo-legal moves and filters illegal moves during search by checking king safety.
- Make/unmake with a move stack (copy-make was measured as too slow)
- Zobrist hashing for the transposition table and repetition detection
  
**Search**
- Principal Variation Search with aspiration windows
- Null move pruning, late move reductions
- Quiescence search, static exchange evaluation (SEE) for capture pruning
- Transposition table, move ordering by selection sort
  
**Evaluation**
- Hand-crafted, tapered (middlegame/endgame), piece-square tables, pawn structure with a pawn hash table, king safety
- Adapted selected evaluation terms from [Ethereal](https://github.com/AndyGrant/Ethereal)
- Tuned with texel-tuner on the Zurich 1M-position dataset

## Branches
- `main`: best tested build (Search v1, Evaluation v2)
- `search-rewrite`: development of a cleaner search implementation

## Layout
 
```
src/
 core/       # Board representation, attack tables, hashing, position
 engine/     # Move generation, search, evaluation, transposition table
 ...         # Entry points, cli (debug), uci
```
## Build and run
 
Requirements: CMake, a C++17/20 compiler (MSVC and GCC tested)

```
mkdir build && cd build
cmake -G "Ninja" .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
Supported options:
```
chess_engine.exe <command>
 - cli - launches debug cli
 - uci - launches uci interface
 - testsuite - launches perft tests on predefined positions
 - perft - launches perft test on startpos, depth 5
 - perft <fen> <depth> <expected nodes (optional)> - launches perft on specified fen position to specified depth, providing expected nodes acts like test
```
Example UCI session:
 
```
uci
isready
position startpos moves e2e4 e7e5
go depth 15
```

## Design decisions
 
- **Make/unmake over copy-make:** copying the whole position per move was too slow in practice, so the engine keeps a move stack and undoes moves.
- **Pseudo-legal move generation:** legality is checked only for moves that are actually searched, which is cheaper than filtering every move up front.
- **Hybrid board state:** bitboards for attacks and set operations, an array for "what piece is on this square" lookups.
- **Compile-time attack tables:** most are `consteval`; the rook table is `constexpr` static storage because MSVC will not evaluate ~135k elements at compile time (without raising MSVC’s /constexpr:steps limit).

## Roadmap

- Search rewrite in progress on the `search-rewrite` branch (cleaner structure, fewer features so far); it will be merged once it matches `main` in head-to-head testing
- Testing routine planned: `bench` command, larger perft suite, SPRT testing for each change
- Tuning process needs more work (texel-tuner tends to change piece-square values rather than evaluation terms)

## Acknowledgements
 
Evaluation terms adapted from Ethereal by Andrew Grant; parameters tuned with texel-tuner on the Zurich dataset.
