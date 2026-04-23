# Overview
High performance, UCI compliant chess engine written in C++ 17. <br/>
Currently rated at approximately 2300 Elo, project focuses on high-speed bitboard manipulation and a optimized Alpha-Beta search.

# Technical Specifications
## Board Representation
- Bitboards: Uses a hybrid bitboard, mailbox architecture for state representation.
- Move Generation: Implements Magic Bitboards for near-instant calculation of sliding piece attacks (Rooks, Bishops, Queens).
- Zobrist Hashing: Efficient position keys for Transposition Table indexing and threefold repetition detection.

## Search Engine
The search is built on a PVS (Principal Variation Search) framework with several aggressive pruning and reduction techniques:
- Aspiration Windows: To narrow search bounds and increase throughput.
- Null Move Pruning (NMP): To quickly prune branches that offer no threat.
- LMR & LRM: Late Move Reductions and Late Move Transitions to focus depth on the most promising lines.
- Quiescence Search (QS): Mitigates the horizon effect by resolving captures and checks.
- SEE (Static Exchange Evaluation): Prunes bad captures directly at the node level.
- Transposition Table (TT): Global caching of evaluated positions and move ordering.

## Evaluation
Unlike many modern engines using NNUE, this project uses a Pure Hand-Crafted Evaluation. This allows for transparent, human-readable chess logic:
- Tapered Eval: Smoothly transitions between Middlegame and Endgame weights.
- PSQT: Advanced Piece-Square Tables for positional awareness.
- Pawn Theory: Specialized PawnTable for evaluating structure, passers, and weaknesses.

## Building
```bash
  mkdir build && cd build
  cmake -G "Ninja" .. -DCMAKE_BUILD_TYPE=Release
  cmake --build .
```

## Example UCI commands
```bash
  uci
  isready
  position startpos moves e2e4 e7e5
  go depth 15
```
