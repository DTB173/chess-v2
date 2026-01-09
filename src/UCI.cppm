module;
#include <string>
#include <iostream>
#include <sstream>
export module UCI;
import Position;
import Types;
import Zobrist;
import Perft;
import Search;

export namespace UCI {
	enum class OpType {
		ID,
		ISREADY,
		NEWGAME,
		POSITION,
		GO,
		QUIT,
		NONE,
	};

	OpType parse_input(std::string_view input) {
		if (input == "uci") return OpType::ID;
		if (input == "isready") return OpType::ISREADY;
		if (input == "ucinewgame") return OpType::NEWGAME;
		if (input.starts_with("position")) return OpType::POSITION;
		if (input.starts_with("go")) return OpType::GO;
		if (input == "quit") return OpType::QUIT;
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
			int promoBase = MoveFlags::Promo;
			// If it was already a capture, flags is 0b0100... 
			// Adding Promo (0b1000...) makes it 0b1100... (a Promo-Capture)

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
			pos.init_start_pos();
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
		Search::Searcher searcher;
		Position::Position pos;
		OpType op{ OpType::NONE };
		std::string input;
		bool play{ true };
		while (play && std::getline(std::cin, input)) {
			op = parse_input(input);

			switch (op) {
			case OpType::ID:
				std::cout << "id name MyChessEngine\n";
				std::cout << "id author MyName\n";
				std::cout << "uciok\n";
				break;
			case OpType::ISREADY:
				std::cout << "readyok\n";
				break;
			case OpType::NEWGAME:
				Search::tt.clear();

				break;
			case OpType::POSITION: handle_position(input, pos); break;
			case OpType::GO: {
				int depth = 12;
				// Parse depth if provided
				if (auto depth_pos = input.find("depth "); depth_pos != std::string::npos) {
					depth = std::stoi(input.substr(depth_pos + 6));
				}
				Types::Move best_move = searcher.start_search(pos, depth);
				std::cout << "bestmove " << Types::move_to_string(best_move) << "\n";
				break;
			}
			case OpType::QUIT:play = false; break;
			}
		}
	}
}