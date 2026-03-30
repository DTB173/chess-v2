#pragma once

#include <iostream>
#include <algorithm>
#include <string>
#include <string_view>

#include "Position.h"
#include "Types.h"
#include "Zobrist.h"
#include "MoveGen.h"
#include "Perft.h"
#include "Search.h"

namespace CLI {
	constexpr const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	constexpr const char* test = "8/8/8/8/8/8/ppp5/K6k b - - 0 1";

	using namespace Types;
	enum class OpType {
		PLAY,
		PRINT,
		QUIT,
		UNDO,
		MOVES,
		LIST,
		DIVIDE,
		NONE
	};
	OpType parse_input(std::string_view in) {
		if (in == "print") return OpType::PRINT;
		else if (in == "quit") return OpType::QUIT;
		else if (in == "undo") return OpType::UNDO;
		else if (in == "movelist") return OpType::LIST;
		else if (in.find("divide") != std::string::npos) return OpType::DIVIDE;
		else if (in.find("play") != std::string::npos) return OpType::PLAY;
		else if (in.find("moves") != std::string::npos) return OpType::MOVES;
		return OpType::NONE;
	}

	Square square_from_notation(const char* sq) {
		int file = sq[0] - 'a';
		int rank = sq[1] - '1';
		return Square::from_rf(rank, file);
	}

	Move parse_move(const char* notation, const Position::Position& pos) {
		using namespace Types;

		Square from = square_from_notation(notation);
		Square to = square_from_notation(notation + 2);

		const Piece p = pos.get_mailbox()[from];
		if (p.is_none()) return Move(); // Sanity check

		PieceType type = p.type();
		int flags = MoveFlags::Quiet;

		// 1. Pawn Special Moves
		if (type == PieceType::PAWN) {
			int dist = std::abs(from.rank() - to.rank());

			if (dist == 2) {
				flags = MoveFlags::DoublePush;
			}
			// If moving diagonally to an empty square, it's En Passant
			else if (from.file() != to.file() && pos.get_mailbox()[to].is_none()) {
				flags = MoveFlags::EnPassant;
			}
		}

		// 2. Castling
		// If the king moves more than 1 file, it's a castle
		if (type == PieceType::KING && std::abs(from.file() - to.file()) > 1) {
			flags = MoveFlags::Castling;
		}

		// 3. Promotions
		// notation[4] is the 5th character (e.g., 'q' in "e7e8q")
		if (notation[4] != '\0' && notation[4] != ' ' && notation[4] != '\n' && notation[4] != '\r') {
			switch (notation[4]) {
			case 'q': flags = MoveFlags::PromoQueen;  break;
			case 'n': flags = MoveFlags::PromoKnight; break;
			case 'r': flags = MoveFlags::PromoRook;   break;
			case 'b': flags = MoveFlags::PromoBishop; break;
			}
		}

		return Types::Move(from, to, flags);
	}

	void game_loop() {
		Zobrist::init();
		Search::Searcher searcher;
		Position::Position pos;
		if (!pos.set_fen(startpos)) {
			std::cerr << "Error laoding fen\n";
			return;
		}
		pos.print(std::cout);
		bool playing{ true };
		OpType operation{ OpType::NONE };
		std::string input;
		int move_count = 0;
		while (playing) {
			if (pos.turn() == Types::Color::BLACK && operation != OpType::UNDO) {
				Types::Move m = searcher.start_search(pos, 64, 15);
				std::cout << "Best move: " << Types::move_to_string(m) << '\n';
				pos.make_move(m);
				pos.print(std::cout);
				++move_count;
			}

			std::cout << ">> ";
			std::getline(std::cin, input);
			operation = parse_input(input);


			switch (operation) {
			case OpType::QUIT: playing = false; break;
			case OpType::UNDO: if (move_count > 2)pos.undo_move(); pos.undo_move(); pos.print(std::cout); break;
			case OpType::PRINT:pos.print(std::cout); break;
			case OpType::PLAY: {
				std::string cmd, arg;
				if (auto space_pos = input.find(' '); space_pos != std::string::npos) {
					cmd = input.substr(0, space_pos);
					arg = input.substr(space_pos + 1);
					Types::Move m = parse_move(arg.data(), pos);
					if (Position::is_move_legal(pos, m)) {
						pos.make_move(m);
						++move_count;
					}
					else {
						std::cerr << "Invalid move!\n";
					}
					pos.print(std::cout);
				}
				else {
					std::cerr << "Invalid move notation, command was: " << cmd << "arg was: " << arg << '\n';
				}
			}break;
			case OpType::MOVES: {
				std::string cmd, arg;
				if (auto space_pos = input.find(' '); space_pos != std::string::npos) {
					cmd = input.substr(0, space_pos);
					arg = input.substr(space_pos + 1);
					Types::Square sq = square_from_notation(arg.data());
					pos.highlight_attacks(std::cout, sq);
				}
				else {
					std::cerr << "Invalid move notation, command was: " << cmd << "arg was: " << arg << '\n';
				}
			}break;
			case OpType::LIST: {
				MoveGen::MoveList moves{};
				MoveGen::generate_all_moves(pos, moves);
				std::cout << "Legal moves:\n";
				for (const auto& m : moves) {
					Types::Color us = pos.get_metadata().side_to_move();
					pos.make_move(m);
					if (!pos.is_square_attacked(pos.get_king_square(us), us)) {
						std::cout << Types::move_to_string(m) << '\n';
					}
					pos.undo_move();
				}
			}break;
			case OpType::DIVIDE: {
				std::string arg;
				if (auto space_pos = input.find(' '); space_pos != std::string::npos) {
					arg = input.substr(space_pos + 1);
					int depth = std::stoi(arg);
					Perft::divide(pos, depth);
				}
			} break;
			default:std::cout << "Unknown command.\n";
			}
		}
	}
}

