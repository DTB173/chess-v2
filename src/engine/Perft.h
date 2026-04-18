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
        MoveGen::MoveList moves;
        MoveGen::generate_all_moves(pos, moves);

        if (depth == 1 && bulk_count) {
            ui64 count = 0;
            for (const auto& m : moves) {
                Color us = pos.turn();
                pos.make_move(m);
                if (!pos.is_in_check(us)) {
                    count++;
                }
                pos.undo_move();
            }
            return count;
        }

        if (depth == 0) return 1;

        ui64 nodes = 0;
        for (const auto& m : moves) {
            Color us = pos.turn();
            pos.make_move(m);

            if (!pos.is_in_check(us)) {
                nodes += perft(pos, depth - 1, bulk_count);
            }

            pos.undo_move();
        }

        return nodes;
    }

    ui64 run_perft(int depth, bool bulk) {
        Zobrist::init();
        Position::Position pos; 
        [[maybe_unused]] constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        [[maybe_unused]] constexpr const char* kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
        pos.set_fen(kiwipete);
        
        return perft(pos, depth, bulk);
    }

    void divide(Position::Position& pos, int depth) {
        MoveGen::MoveList moves;
        MoveGen::generate_all_moves(pos, moves);
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