#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <format>
#include <thread>

#include "Position.h"
#include "Types.h"
#include "Zobrist.h"
#include "Perft.h"
#include "Search.h"

namespace UCI {
	using namespace Types;

	std::mutex thread_mutex;
	std::thread search_thread;

	void stop_and_join() {
		Search::Searcher::stop_signal = true;
		if (search_thread.joinable()) {
			search_thread.join();
		}
	}

	constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	enum class OpType {
		ID,
		ISREADY,
		NEWGAME,
		POSITION,
		GO,
		QUIT,
		STOP,
		NONE,
	};


	OpType parse_input(std::string_view input) {
		if (input == "uci") return OpType::ID;
		if (input == "isready") return OpType::ISREADY;
		if (input == "ucinewgame") return OpType::NEWGAME;
		if (input.starts_with("position")) return OpType::POSITION;
		if (input.starts_with("go")) return OpType::GO;
		if (input == "quit") return OpType::QUIT;
		if (input == "stop") return OpType::STOP;
		return OpType::NONE;
	}

	Square square_from_notation(const char* sq) {
		int file = sq[0] - 'a';
		int rank = sq[1] - '1';
		return Square::from_rf(rank, file);
	}

	Types::Move parse_move(const char* notation, const Position::Position& pos) {
		using namespace Types;

		Square from = square_from_notation(notation);
		Square to = square_from_notation(notation + 2);

		// 1. Identify context from the board
		const Piece p = pos.get_mailbox()[from];
		const Piece target = pos.get_mailbox()[to];
		int flags = MoveFlags::Quiet;

		// 2. Determine if it's a capture
		if (!target.is_none()) {
			flags = MoveFlags::Capture;
		}

		// 3. Handle Special Cases (Pawn/King)
		if (p.type() == PieceType::PAWN) {
			if (std::abs(from.rank() - to.rank()) == 2) {
				flags = MoveFlags::DoublePush;
			}
			else if (from.file() != to.file() && target.is_none()) {
				flags = MoveFlags::EnPassant; // (Note: your EnPassant already includes Capture bit)
			}
		}
		else if (p.type() == PieceType::KING && std::abs(from.file() - to.file()) > 1) {
			flags = MoveFlags::Castling;
		}

		// 4. Handle Promotions
		// notation[4] is the char like 'q', 'r', 'n', 'b'
		char promo = notation[4];
		if (promo >= 'a' && promo <= 'z') {
			switch (promo) {
			case 'n': flags |= MoveFlags::PromoKnight; break;
			case 'b': flags |= MoveFlags::PromoBishop; break;
			case 'r': flags |= MoveFlags::PromoRook;   break;
			case 'q': flags |= MoveFlags::PromoQueen;  break;
			}
		}

		return Types::Move(from, to, flags);
	}

	void handle_position(std::string_view input, Position::Position& pos) {
		Zobrist::init();
		if (input.find("startpos") != std::string::npos) {
			pos.set_fen(startpos);
		}
		else if (auto fen_pos = input.find("fen "); fen_pos != std::string::npos) {
			// Find where moves start to isolate FEN
			auto moves_pos = input.find(" moves");
			std::string fen = (moves_pos == std::string::npos)
				? std::string(input.substr(fen_pos + 4))
				: std::string(input.substr(fen_pos + 4, moves_pos - (fen_pos + 4)));
			pos.set_fen(fen);
		}

		if (auto moves_idx = input.find(" moves "); moves_idx != std::string::npos) {
			std::string moves_str = std::string(input.substr(moves_idx + 7));
			std::stringstream ss(moves_str);
			std::string move_token;

			while (ss >> move_token) {
				Types::Move m = parse_move(move_token.c_str(), pos);
				pos.make_move(m);
			}
		}
	}

	void uci_loop() {
		Zobrist::init();
		Position::Position pos;
		Search::Searcher searcher;

		OpType op{ OpType::NONE };
		std::string input;
		bool play{ true };

		while (play && std::getline(std::cin, input)) {
			op = parse_input(input);

			switch (op) {
			case OpType::ID:
				std::cout << "id name MyChessEngine\n";
				std::cout << "id author MyName\n";
				std::cout << "uciok\n" << std::flush;
				break;

			case OpType::ISREADY:
				std::cout << "readyok\n" << std::flush;
				break;

			case OpType::NEWGAME:
				pos.reset();
				Search::tt.clear();
				Eval::pawns_cache.clear();
				break;

			case OpType::POSITION:
				handle_position(input, pos);
				break;

			case OpType::GO: {
				int max_depth = 64;
				int movetime_ms = -1;
				int wtime = -1, btime = -1, winc = 0, binc = 0;
				int movestogo = -1;
				// 1. Parse all potential arguments
				std::stringstream ss(input);
				std::string token;
				while (ss >> token) {
					if (token == "depth") ss >> max_depth;
					else if (token == "movetime") ss >> movetime_ms;
					else if (token == "wtime") ss >> wtime;
					else if (token == "btime") ss >> btime;
					else if (token == "winc") ss >> winc;
					else if (token == "binc") ss >> binc;
					else if (token == "movestogo") ss >> movestogo;
				}

				int time_for_move_ms = 0;

				// 2. Decide how much time to use
				if (movetime_ms != -1) {
					// If GUI says "movetime", we must use exactly that (minus buffer)
					time_for_move_ms = movetime_ms - 50;
				}
				else if (wtime != -1 || btime != -1) {
					bool is_white = pos.turn() == Types::Color::WHITE;
					int time_left = is_white ? wtime : btime;
					int inc = is_white ? winc : binc;

					// Use total plies to find move number
					int current_move = pos.half_move_count() / 2;

					// 1. Dynamic Divisor: Target Move 40 for the bank drain.
					// At Move 1, divisor is 40. At Move 30, divisor is 10.
					int divisor = std::max(10, 40 - current_move);

					// 2. Base Calculation
					// We spend a larger chunk of the bank and 90% of the increment.
					double base_time = (double)time_left / divisor;
					time_for_move_ms = (int)(base_time + (inc * 0.9));

					// 3. Middle Game "Combat" Bonus (Moves 10-30)

					if (current_move >= 10 && current_move <= 30) {
						time_for_move_ms = (int)(time_for_move_ms * 1.25);
					}
					// 4. Safety Buffer
					time_for_move_ms -= 50;
				}
				else {
					// Fallback for "go" with no arguments (e.g. 15 seconds)
					time_for_move_ms = 1500000;
				}

				time_for_move_ms = std::max(80, time_for_move_ms - 120);
				std::cout << "info string allocated " << time_for_move_ms << "ms\n" << std::flush;
				stop_and_join();

				Search::Searcher::stop_signal = false;

				search_thread = std::thread([&searcher, &pos, max_depth, time_for_move_ms]() {
					Types::Move best_move = searcher.start_search(pos, max_depth, time_for_move_ms / 1000.0);
					std::string move_str = (best_move == Types::NO_MOVE) ? "0000" : Types::move_to_string(best_move);
					std::string out = std::format("bestmove {}\n", move_str);
					std::cout << out << std::flush;
					});
			}
						   break;
			case OpType::QUIT:
				stop_and_join();
				play = false;
				break;
			case OpType::STOP:
				stop_and_join();
				break;
			default:
				break;
			}
		}
	}
}