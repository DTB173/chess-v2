#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <format>
#include <thread>
#include <mutex>

#include "Position.h"
#include "Types.h"
#include "Zobrist.h"
#include "Perft.h"
#include "Searchv2.h"

namespace UCI {
	using namespace Types;

	constexpr size_t npos = std::numeric_limits<size_t>::max();
	std::thread search_thread;

	struct EngineOptions { int64_t hash_mb = 64; int threads = 1; };

	struct GoParams {
		int64_t wtime = -1, btime = -1, winc = 0, binc = 0;
		int64_t movetime = -1, movestogo = -1, nodes = -1;
		int     depth = Search::Search_Constants::MAX_PLY - 1;
		bool    infinite = false;
	};

	TimeLimits compute_limits(const GoParams& g, bool white) {
		constexpr int64_t overhead = 30;
		if (g.movetime >= 0) { auto t = std::max<int64_t>(1, g.movetime - overhead); return { t, t }; }

		int64_t left = white ? g.wtime : g.btime;
		int64_t inc = white ? g.winc : g.binc;
		if (left < 0 || g.infinite) return { INT64_MAX, INT64_MAX };

		int64_t avail = std::max<int64_t>(1, left - overhead);
		int64_t mtg = g.movestogo > 0 ? std::min<int64_t>(g.movestogo, 50) : 30;
		int64_t soft = std::min(avail / mtg + inc * 3 / 4, avail / 2);
		int64_t hard = std::min(soft * 4, avail * 3 / 4);
		return { std::max<int64_t>(1, soft), std::max<int64_t>(1, hard) };
	}


	void stop_and_join() {
		Search::Searcher::stop_signal = true;
		if (search_thread.joinable()) {
			search_thread.join();
		}
	}

	constexpr const std::string_view startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	enum class OpType {
		ID,
		ISREADY,
		NEWGAME,
		POSITION,
		GO,
		OPTION,
		QUIT,
		STOP,
		NONE,
	};

	OpType parse_input(std::string_view input) {
		if (input == "uci")                return OpType::ID;
		if (input == "isready")            return OpType::ISREADY;
		if (input == "ucinewgame")         return OpType::NEWGAME;
		if (input.starts_with("position")) return OpType::POSITION;
		if (input.starts_with("go"))       return OpType::GO;
		if (input.starts_with("option"))   return OpType::OPTION;
		if (input == "quit")               return OpType::QUIT;
		if (input == "stop")               return OpType::STOP;
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
				flags = MoveFlags::EnPassant;
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

	GoParams handle_go(const std::string& input) {
		GoParams options{};
		std::stringstream ss(input);
		std::string token;
		while (ss >> token) {
			if (token == "depth") ss >> options.depth;
			else if (token == "movetime") ss >> options.movetime;
			else if (token == "wtime") ss >> options.wtime;
			else if (token == "btime") ss >> options.btime;
			else if (token == "winc") ss >> options.winc;
			else if (token == "binc") ss >> options.binc;
			else if (token == "movestogo") ss >> options.movestogo;
			else if (token == "infinite") options.infinite = true;
		}
		return options;
	}

	void handle_option() {

	}

	void uci_loop() {
		Zobrist::init();
		Position::Position pos;
		Search::Searcher searcher;
		EngineOptions engine_params{};
		searcher.tt.resize(engine_params.hash_mb);

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
				stop_and_join();
				pos.reset();
				searcher.clear();
				break;

			case OpType::POSITION:
				stop_and_join();
				handle_position(input, pos);
				break;

			case OpType::GO: {
				auto params = handle_go(input);

				bool white_move = pos.turn() == Color::WHITE;
				auto limits = compute_limits(params, white_move);

				stop_and_join();
				Search::Searcher::stop_signal = false;

				int64_t depth = params.depth;

				if(limits.hard_ms != npos) std::cout << "[info] string allocated " << limits.hard_ms << "ms\n" << std::flush;

				search_thread = std::thread([&searcher, &pos, depth, limits]() {
					Types::Move best_move = searcher.start_search(pos, depth, limits);
					std::string move_str = (best_move == Types::NO_MOVE) ? "0000" : Types::move_to_string(best_move);
					std::string out = std::format("bestmove {}\n", move_str);
					std::cout << out << std::flush;
					}
				);
			}
			break;
			case OpType::OPTION: {
				handle_option();
				break;
			}
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
		stop_and_join();
	}
}