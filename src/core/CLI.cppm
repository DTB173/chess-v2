module;
#include <iostream>
#include <algorithm>
#include <string>
#include <string_view>

export module CLI;
import Position;
import Types;

export namespace CLI {
	enum class OpType{
		PLAY,
		PRINT,
		QUIT,
		UNDO,
		MOVES,
		NONE
	};
	OpType parse_input(std::string_view in) {
		if (in == "print") return OpType::PRINT;
		else if (in == "quit") return OpType::QUIT;
		else if (in == "undo") return OpType::UNDO;
		else if (in.find("play") != std::string::npos) return OpType::PLAY;
		else if (in.find("moves") != std::string::npos) return OpType::MOVES;
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

		const Piece p = pos.get_mailbox()[from];
		PieceType type = p.type();
		Color us = p.color();

		int flags = MoveFlags::Quiet;

		if (type == PieceType::PAWN) {
			int rank_from = from.rank();
			int rank_to = to.rank();

			if (std::abs(rank_from - rank_to) == 2) {
				flags = MoveFlags::DoublePush;
			}
			else if (from.file() != to.file() && pos.get_mailbox()[to].is_none()) {
				int ep_file = pos.get_metadata().en_passant_file();
				if (ep_file >= 0 && ep_file == to.file()) {
					flags = MoveFlags::EnPassant;
				}
			}
		}
		if (type == PieceType::KING) {
			if (std::abs((from % 8) - (to % 8)) > 1) {
				flags = MoveFlags::Castling;
			}
		}
		if (notation[4] != '\0' && notation[4] != ' ') {
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
		Position::Position pos;
		pos.init_start_pos();
		pos.print(std::cout);
		bool playing{ true };
		OpType operation{OpType::NONE};
		std::string input;

		while (playing) {
			std::cout << ">> ";
			std::getline(std::cin, input);
			operation = parse_input(input);

			switch (operation) {
			case OpType::QUIT: playing = false; break;
			case OpType::UNDO: pos.undo_move(); pos.print(std::cout); break;
			case OpType::PRINT:pos.print(std::cout); break;
			case OpType::PLAY: {
				std::string cmd, arg;
				if (auto space_pos = input.find(' '); space_pos!=std::string::npos) {
					cmd = input.substr(0, space_pos);
					arg = input.substr(space_pos + 1);
					Types::Move m = parse_move(arg.data(), pos);
					if (Position::is_move_legal(pos, m)) {
						pos.make_move(m);
					}
					else {
						std::cerr << "Invalid move!\n";
					}
					pos.print(std::cout);
				}
				else {
					std::cerr << "Invalid move notation, command was: " << cmd << "arg was: "<< arg<<'\n';
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
			default:std::cout << "Unknown command.\n";
			}
		}
	}
}

