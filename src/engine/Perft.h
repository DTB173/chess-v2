#pragma once

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <array>
#include <vector>
#include <format>

#if defined(_MSC_VER)
#include <io.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

#include "Types.h"
#include "Position.h"
#include "MoveGen.h"
#include "Zobrist.h"

namespace Perft
{
    using namespace Types;

    namespace Console {
        // ANSI Escape Codes
        const std::string GREEN = "\033[32m";
        const std::string RED   = "\033[31m";
        const std::string BOLD  = "\033[1m";
        const std::string RESET = "\033[0m";
    }

    inline bool supports_color() {
        static bool checked = []() {
#if defined(_MSC_VER)
            return _isatty(_fileno(stdout)) != 0;
#else
            return isatty(STDOUT_FILENO) != 0;
#endif
            }();
        return checked;
    }

    inline std::string format_color(const std::string& color, const std::string& msg, bool bold = true) {
        if (!supports_color()) {
            return msg;
        }
        return color + (bold ? Console::BOLD : "") + msg + Console::RESET;
    }

    struct PerftRecord {
        std::string fen;
        int depth;
        std::optional<size_t> expected_nodes;
    };

    PerftRecord parse_record(int argc, char* argv[]) {
        PerftRecord record{};

        if (argc < 3) {
            record.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            record.depth = 5;
            return record;
        }

        std::string arg2 = argv[2];

        if (arg2 == "startpos") {
            record.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        }
        else {
            record.fen = arg2;
        }

        if (argc > 3) {
            try {
                record.depth = std::stoi(argv[3]);
            }
            catch (...) {
                record.depth = 5;
            }
        }
        else {
            record.depth = 5;
        }

        if (argc > 4) {
            try {
                std::string exp_str = argv[4];
                exp_str.erase(
                    std::remove_if(exp_str.begin(), exp_str.end(), [](char c) {
                        return c == '\'' || c == ',';
                        }),
                    exp_str.end()
                );
                record.expected_nodes = std::stoull(exp_str);
            }
            catch (...) { }
        }

        return record;
    }

    ui64 perft(Position::Position& pos, int depth, bool bulk = true) {
        if (depth == 0) return 1;
        MoveGen::MoveList moves;
        MoveGen::generate_all_moves(pos, moves);

        if (depth == 1 && bulk) {
            ui64 n = 0;
            for (const auto& m : moves) n += pos.is_legal(m);
            return n;
        }
        ui64 nodes = 0;
        for (const auto& m : moves) {
            if (!pos.is_legal(m)) continue;
            pos.make_move(m);
            nodes += perft(pos, depth - 1, bulk);
            pos.undo_move();
        }
        return nodes;
    }

    ui64 run_perft(const PerftRecord& record, bool bulk = true) {
        Zobrist::init();
        Position::Position pos;

        pos.set_fen(record.fen);
        
        return perft(pos, record.depth, bulk);
    }

    void divide(Position::Position& pos, int depth) {
        MoveGen::MoveList moves;
        MoveGen::generate_all_moves(pos, moves);
        std::vector<std::pair<std::string, int>> moves_str;
        moves_str.reserve(moves.size());
        ui64 total_nodes = 0;
        for (const auto& m : moves) {
            Color us = pos.get_metadata().side_to_move();
            pos.make_move(m);
            if (!pos.is_square_attacked(pos.get_king_square(us), us) && depth >= 1) {
                ui64 nodes = perft(pos, depth - 1);
                moves_str.emplace_back(std::pair{ Types::move_to_string(m), nodes });
                total_nodes += nodes;
            }
            pos.undo_move();
        }
        std::sort(moves_str.begin(), moves_str.end(), [](const auto& left, const auto& right) {
            return left.first < right.first;
            });

        for (const auto& [move, count] : moves_str) {
            std::cout << move << ": " << count << '\n';
        }
        std::cout << "\nTotal: " << total_nodes << std::endl;
    }

    bool runt_test_suite() {
        static const std::array<PerftRecord, 6> test_suite = {
            PerftRecord{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",                 5, 4'865'609 },
            PerftRecord{ "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",     4, 4'085'603 },
            PerftRecord{ "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                                5, 674'624 },
            PerftRecord{ "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",         4, 422'333 },
            PerftRecord{ "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",                4, 2'103'487 },
            PerftRecord{ "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3'894'594 }
        };

        int count_passed = 0;
        int total_evaluated = 0;

        for (const auto& record : test_suite) {
            auto nodes = run_perft(record, false);

            std::cout << "==========================================================================\n";
            std::cout << "Position fen:   " << record.fen << '\n';
            std::cout << "Depth:          " << record.depth << '\n';
            std::cout << "Nodes measured: " << nodes << '\n';

            if (record.expected_nodes.has_value()) {
                total_evaluated++;
                std::cout << "Nodes expected: " << *record.expected_nodes << '\n';

                bool passed = (*record.expected_nodes == nodes);
                if (passed) {
                    std::cout << "Result:         " << format_color(Console::GREEN, "PASSED") << '\n';
                    count_passed++;
                }
                else {
                    std::cout << "Result:         " << format_color(Console::RED, "FAILED") << '\n';
                }
            }
            else {
                std::cout << "Result:         No expected nodes provided\n";
            }
            std::cout << "==========================================================================\n\n";
        }

        std::cout << Console::BOLD << "Test Suite Summary: ";
        if (count_passed == total_evaluated) {
            std::cout << format_color(Console::GREEN, std::format("{}/{} tests passed successfully!",count_passed, total_evaluated )) << '\n';
        }
        else {
            std::cout << format_color(Console::RED, std::format("{}/{} tests passed.", count_passed, total_evaluated)) << '\n';
        }

        return count_passed == total_evaluated;
    }

    void measure_perft(const PerftRecord& record) {
		std::cout << "=========== PERFT  ===========\n";
        auto start = std::chrono::high_resolution_clock::now();

        ui64 nodes = Perft::run_perft(record);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        double nps = nodes / elapsed.count();

        std::cout << "Fen:            " << record.fen << '\n';
        std::cout << "Depth:          " << record.depth << "\n";
        std::cout << "Nodes measured: " << nodes << "\n";
        if (record.expected_nodes.has_value()) {
            std::cout << "Nodes expected: " << *record.expected_nodes << '\n';
            bool passed = nodes == *record.expected_nodes;
            const std::string& color = (passed ? Console::GREEN : Console::RED);
            std::string result = passed ? "PASSED" : "FAILED";
            std::cout << "Result:         " << format_color(color, result) << '\n';
        }
        std::cout << "Time:           " << elapsed.count() << "s\n";
        std::cout << "NPS:            " << static_cast<ui64>(nps) << "\n";
		std::cout << "===============================\n\n";
    }
}