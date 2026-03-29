#pragma once

#include <chrono>
#include <iostream>

#include "Types.h"
#include "Position.h"
#include "MoveGen.h"
#include "Zobrist.h"

namespace Perft
{
    using namespace Types;

    ui64 perft(Position::Position& pos, int depth, bool bulk_count = true) {
        // Generate moves for the current position
        auto moves = MoveGen::generate_all_moves(pos);

        // 1. BULK COUNTING OPTIMIZATION
        // If we are at depth 1 and bulk_count is enabled, we need to return
        // the count of LEGAL moves. 
        if (depth == 1 && bulk_count) {
            ui64 count = 0;
            for (const auto& m : moves) {
                Color us = pos.get_metadata().side_to_move();
                pos.make_move(m);
                // Only count if the move didn't leave the king in check
                if (!pos.is_square_attacked(pos.get_king_square(us), us)) {
                    count++;
                }
                pos.undo_move();
            }
            return count;
        }

        // 2. BASE CASE (If bulk counting is off or we hit depth 0)
        if (depth == 0) return 1;

        // 3. RECURSIVE STEP
        ui64 nodes = 0;
        for (const auto& m : moves) {
            Color us = pos.get_metadata().side_to_move();
            pos.make_move(m);

            // Legal move check
            if (!pos.is_square_attacked(pos.get_king_square(us), us)) {
                nodes += perft(pos, depth - 1, bulk_count);
            }

            pos.undo_move();
        }

        return nodes;
    }

    ui64 run_perft(int depth, bool bulk) {
        Zobrist::init();
        Position::Position pos; 
        constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        pos.set_fen(startpos);
        
        return perft(pos, depth, bulk);
    }

    void divide(Position::Position& pos, int depth) {
        auto moves = MoveGen::generate_all_moves(pos);
        ui64 total_nodes = 0;
        for (const auto& m : moves) {
            Color us = pos.get_metadata().side_to_move();
            pos.make_move(m);
            if (!pos.is_square_attacked(pos.get_king_square(us), us)) {
                ui64 nodes = perft(pos, depth - 1);
                std::cout << Types::move_to_string(m) << ": " << nodes << std::endl;
                total_nodes += nodes;
            }
            pos.undo_move();
        }
        std::cout << "\nTotal: " << total_nodes << std::endl;
    }

	// Performance measurement utility (startpos)
    void measure_perft(int depth, bool bulk) {
		std::cout << "=========== PERFT " << depth << " ===========\n";
        auto start = std::chrono::high_resolution_clock::now();

        ui64 nodes = Perft::run_perft(depth, bulk);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        double nps = nodes / elapsed.count();

        std::cout << "Depth: " << depth << "\n";
        std::cout << "Nodes: " << nodes << "\n";
        std::cout << "Time:  " << elapsed.count() << "s\n";
        std::cout << "NPS:   " << static_cast<ui64>(nps) << "\n";
		std::cout << "===============================\n\n";
    }
}