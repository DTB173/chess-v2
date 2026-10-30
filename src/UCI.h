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

#include "engine/Evaluationv2.h"

namespace UCI {
	using namespace Types;

	namespace {
		constexpr std::array<std::string_view,25> BENCH_FENS = {
			"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
			"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
			"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
			"4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
			"rq3rk1/ppp2ppp/1bnpN3/3N2B1/4P3/7P/PPPQ1PP1/2KR3R b - - 0 14",
			"r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4PpP1/1BNP4/PPP2P1P/3R1RK1 b - g3 0 14",
			"r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 1",
			"r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
			"r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
			"4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
			"2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
			"r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
			"3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
			"r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
			"4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
			"3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
			"6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
			"3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
			"2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 4 3",
			"8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 ",
			"7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
			"8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
			"8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
			"8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
			"8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1"
		};
	}

	constexpr size_t npos = std::numeric_limits<size_t>::max();
	std::thread search_thread;

	struct EngineOptions { int64_t hash_mb = 256; int threads = 1; };

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

	constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	enum class OpType {
		ID,
		ISREADY,
		NEWGAME,
		POSITION,
		GO,
		OPTION,
		QUIT,
		STOP,
		BENCH,
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
		if (input == "bench")			   return OpType::BENCH;
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

	void handle_position(const std::string& input, Position::Position& pos) {
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

	void handle_bench(Position::Position& pos, Search::Searcher& searcher, int depth = 9) {
		struct Row { std::string_view fen; uint64_t nodes; double ms; };
		std::vector<Row> rows;
		rows.reserve(BENCH_FENS.size());

		TimeLimits limits{INT_MAX, INT_MAX};
		for (std::string_view fen : BENCH_FENS) {
			pos.reset();
			pos.set_fen(std::string(fen));
			searcher.clear();
			searcher.tt.reset_stats();
			Eval::pawns_cache.reset_stats();
			auto t0 = std::chrono::steady_clock::now();
			search_thread = std::thread([&searcher, &pos, depth, limits]() {
				Types::Move best_move = searcher.start_search(pos, depth, limits);
				std::string move_str = (best_move == Types::NO_MOVE) ? "0000" : Types::move_to_string(best_move);
				std::cout << std::format("bestmove {}\n", move_str) << std::flush;
				}
			);
			search_thread.join();
			searcher.tt.print_stats();
			Eval::pawns_cache.print_stats();
			auto t1 = std::chrono::steady_clock::now();
			rows.push_back({ fen, searcher.nodes_visited, std::chrono::duration<double, std::milli>(t1 - t0).count() });
		}

		uint64_t total_nodes = 0;
		double total_ms = 0;
		std::printf("\n%-3s %12s %10s %10s  %s\n", "#", "nodes", "ms", "knps", "fen");
		for (size_t i = 0; i < rows.size(); ++i) {
			const Row& r = rows[i];
			total_nodes += r.nodes;
			total_ms += r.ms;
			std::printf("%-3zu %12llu %10.1f %10.0f  %.*s\n", i + 1,
				(unsigned long long)r.nodes, r.ms,
				r.ms > 0 ? r.nodes / r.ms : 0.0,
				(int)r.fen.size(), r.fen.data());
		}
		std::printf("\n===========================\n");
		std::printf("Total time (ms) : %.0f\n", total_ms);
		std::printf("Nodes searched  : %llu\n", (unsigned long long)total_nodes);
		std::printf("Nodes/second    : %.0f\n", total_ms > 0 ? total_nodes / (total_ms / 1000.0) : 0.0);
		std::fflush(stdout);
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

				search_thread = std::thread([&searcher, &pos, depth, limits]() {
					Types::Move best_move = searcher.start_search(pos, depth, limits);
					std::string move_str = (best_move == Types::NO_MOVE) ? "0000" : Types::move_to_string(best_move);
					std::cout << std::format("bestmove {}\n", move_str) << std::flush;
					}
				);
			}
			break;
			case OpType::OPTION: {
				handle_option();
				break;
			}
			case OpType::BENCH: {
				int depth = 9;
				handle_bench(pos, searcher, depth);
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