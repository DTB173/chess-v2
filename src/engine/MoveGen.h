#pragma once

#include <array>
#include <cassert>

#include "Types.h"
#include "Bitboards.h"
#include "AttackTables.h"

namespace MoveGen {
	using namespace Types;

	constexpr int MAX_MOVES = 256;

	class MoveList {
	private:
		std::array<Move, MAX_MOVES> moves;
		size_t count{ };
	public:
		using iterator = decltype(moves.begin());
		using const_iterator = decltype(moves.cbegin());

		iterator begin() noexcept { return moves.begin(); }
		iterator end() noexcept { return moves.begin() + count; }

		const_iterator begin()const noexcept { return moves.begin(); }
		const_iterator end()const noexcept { return moves.begin() + count; }
		
		Move& operator[](int idx) { assert(idx < static_cast<int>(count)); return moves[idx]; }
		const Move& operator[](int idx)const { assert(idx < static_cast<int>(count)); return moves[idx]; }

		void push_back(const Move& move) { assert(static_cast<int>(count) < MAX_MOVES); moves[count++] = move; }
		size_t size() const noexcept { return count; }
		void clear() noexcept { count = 0; }
	};

	enum class MoveType {
		QUIET,
		CAPTURE,
		ALL,
	};

	// General serialization for pieces
	inline void serialize(MoveList& list, int from, ui64 targets, int flags) {
		while (targets) {
			int to = Bitwise::pop_lsb(targets);
			list.push_back(Move(from, to, flags));
		}
	}

	// Special case for parrarel pawns
	inline void serialize_relative(MoveList& list, ui64 targets, int shift, int flags) {
		while (targets) {
			int to = Bitwise::pop_lsb(targets);
			int from = to - shift;              
			list.push_back(Move(from, to, flags));
		}
	}

	// Special case for pawns: they need to generate 4 moves per target square
	inline void serialize_promotions(MoveList& list, int from, ui64 targets, bool is_capture) {
		while (targets) {
			int to = Bitwise::pop_lsb(targets);
			int base = is_capture ? MoveFlags::Capture : MoveFlags::Quiet;
			list.push_back(Move(from, to, base | MoveFlags::PromoQueen));
			list.push_back(Move(from, to, base | MoveFlags::PromoKnight));
			list.push_back(Move(from, to, base | MoveFlags::PromoRook));
			list.push_back(Move(from, to, base | MoveFlags::PromoBishop));
		}
	}

	// Special case for parrarel pawns: they need to generate 4 moves per target square
	inline void serialize_promotions_relative(MoveList& list, ui64 targets, int shift, bool is_capture) {
		while (targets) {
			int to = Bitwise::pop_lsb(targets);
			int from = to - shift;
			int base = is_capture ? MoveFlags::Capture : MoveFlags::Quiet;
			list.push_back(Move(from, to, base | MoveFlags::PromoQueen));
			list.push_back(Move(from, to, base | MoveFlags::PromoKnight));
			list.push_back(Move(from, to, base | MoveFlags::PromoRook));
			list.push_back(Move(from, to, base | MoveFlags::PromoBishop));
		}
	}

	template<MoveType T>
	void generate_pawn_moves(const Position::Position& pos, MoveList& moves) {
		constexpr ui64 RANK_1 = 0x00000000000000FFULL; // promo black
		constexpr ui64 RANK_3 = 0x0000000000FF0000ULL; // double push white
		constexpr ui64 RANK_6 = 0x0000FF0000000000ULL; // double push black
		constexpr ui64 RANK_8 = 0xFF00000000000000ULL; // promo white
	
		Color us    = pos.get_metadata().side_to_move();
		ui64  empty = ~(pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK));
		ui64  enemy = pos.get_occupancy(opponent_of(us));
		ui64  pawns = pos.get_bitboard(PieceType::PAWN, us);

		ui64 PROMO_RANK = (us == Color::WHITE ? RANK_8 : RANK_1);
		int forward = (us == Color::WHITE ? 8 : -8);

		if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
			ui64 single_push = (us == Color::WHITE ? pawns << 8 : pawns >> 8) & empty;

			ui64 promos = single_push & PROMO_RANK;
			ui64 regulars = single_push & ~PROMO_RANK;

			serialize_relative(moves, regulars, forward, MoveFlags::Quiet);
			serialize_promotions_relative(moves, promos, forward, false);
			ui64 double_push = (us == Color::WHITE ? 
									(regulars & RANK_3) << 8 : 
									(regulars & RANK_6) >> 8) 
								& empty;

			serialize_relative(moves, double_push, 2 * forward, MoveFlags::DoublePush);
		}

		if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
			ui64 attackers = pawns;
			while (attackers) {
				int from = Bitwise::pop_lsb(attackers);
				ui64 attacks = AttackTables::get_pawn_attacks(from, us);
				ui64 targets = attacks & enemy;

				ui64 promos = targets & PROMO_RANK;
				ui64 normals = targets & ~PROMO_RANK;

				serialize(moves, from, normals, MoveFlags::Capture);
				serialize_promotions(moves, from, promos, true);

				int ep_sq = pos.get_metadata().en_passant_square();
				if (ep_sq != Square::SQ_NONE) {
					if (attacks & (1ULL << ep_sq)) {
						moves.push_back(Move(from, ep_sq, MoveFlags::Capture | MoveFlags::EnPassant));
					}
				}
			}
		}
	}

	template<MoveType T>
	void generate_knight_moves(const Position::Position& pos, MoveList& moves) {
		Color us      = pos.get_metadata().side_to_move();
		ui64  empty   = ~(pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK));
		ui64  enemy   = pos.get_occupancy(opponent_of(us));
		ui64  knights = pos.get_bitboard(PieceType::KNIGHT, us);
		
		while (knights) {
			int from = Bitwise::pop_lsb(knights);
			ui64 targets = AttackTables::get_knight_attacks(from);
			if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
				serialize(moves, from, targets & empty, MoveFlags::Quiet);
			}
			if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
				serialize(moves, from, targets & enemy, MoveFlags::Capture);
			}
		}
	}

	template<MoveType T>
	void generate_bishop_moves(const Position::Position& pos, MoveList& moves) {
		Color us      = pos.get_metadata().side_to_move();
		ui64  occ     = pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK);
		ui64  empty   = ~occ;
		ui64  enemy   = pos.get_occupancy(opponent_of(us));
		ui64  bishops = pos.get_bitboard(PieceType::BISHOP, us);
		
		while (bishops) {
			int from = Bitwise::pop_lsb(bishops);
			ui64 targets = AttackTables::get_bishop_attacks(from, occ);
			if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
				serialize(moves, from, targets & empty, MoveFlags::Quiet);
			}
			if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
				serialize(moves, from, targets & enemy, MoveFlags::Capture);
			}
		}
	}

	template<MoveType T>
	void generate_rook_moves(const Position::Position& pos, MoveList& moves) {
		Color us    = pos.get_metadata().side_to_move();
		ui64  occ   = pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK);
		ui64  empty = ~occ;
		ui64  enemy = pos.get_occupancy(opponent_of(us));
		ui64  rooks = pos.get_bitboard(PieceType::ROOK, us);
		
		while (rooks) {
			int from = Bitwise::pop_lsb(rooks);
			ui64 targets = AttackTables::get_rook_attacks(from, occ);
			if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
				serialize(moves, from, targets & empty, MoveFlags::Quiet);
			}
			if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
				serialize(moves, from, targets & enemy, MoveFlags::Capture);
			}
		}
	}

	template<MoveType T>
	void generate_queen_moves(const Position::Position& pos, MoveList& moves) {
		Color us     = pos.get_metadata().side_to_move();
		ui64  occ    = pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK);
		ui64  empty  = ~occ;
		ui64  enemy  = pos.get_occupancy(opponent_of(us));
		ui64  queens = pos.get_bitboard(PieceType::QUEEN, us);

		while (queens) {
			int from = Bitwise::pop_lsb(queens);
			ui64 targets = AttackTables::get_queen_attacks(from, occ);
			if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
				serialize(moves, from, targets & empty, MoveFlags::Quiet);
			}
			if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
				serialize(moves, from, targets & enemy, MoveFlags::Capture);
			}
		}
	}

	template<MoveType T>
	void generate_king_moves(const Position::Position& pos, MoveList& moves) {
		Color us = pos.turn();
		ui64  empty = ~(pos.get_occupancy(Color::WHITE) | pos.get_occupancy(Color::BLACK));
		ui64  enemy = pos.get_occupancy(opponent_of(us));
		ui64  kings = pos.get_bitboard(PieceType::KING, us);

		while (kings) {
			int from = Bitwise::pop_lsb(kings);
			ui64 targets = AttackTables::get_king_attacks(from);

			if constexpr (T == MoveType::QUIET || T == MoveType::ALL) {
				serialize(moves, from, targets & empty, MoveFlags::Quiet);
				ui64 castle_bits = pos.get_castling_targets(us);
				serialize(moves, from, castle_bits, MoveFlags::Castling);
			}
			if constexpr (T == MoveType::CAPTURE || T == MoveType::ALL) {
				serialize(moves, from, targets & enemy, MoveFlags::Capture);
			}
		}
	}

	void generate_quiet_promotions(const Position::Position& pos, MoveList& moves) {
		constexpr ui64 RANK_1 = 0x00000000000000FFULL;
		constexpr ui64 RANK_8 = 0xFF00000000000000ULL;

		Color us = pos.turn();
		ui64 us_pawns = pos.get_bitboard(PieceType::PAWN, us);
		ui64 empty = ~pos.get_total_occupancy();

		if (us == Color::WHITE) {
			ui64 targets = (us_pawns << 8) & empty & RANK_8;
			serialize_promotions_relative(moves, targets, 8, false);
		}
		else {
			ui64 targets = (us_pawns >> 8) & empty & RANK_1;
			serialize_promotions_relative(moves, targets, -8, false);
		}
	}

	template<MoveType T>
	void generate_moves(const Position::Position & pos, MoveList& moves) {
		generate_queen_moves<T>(pos, moves);
		generate_rook_moves<T>(pos, moves);
		generate_bishop_moves<T>(pos, moves);
		generate_knight_moves<T>(pos, moves);
		generate_pawn_moves<T>(pos, moves);
		generate_king_moves<T>(pos, moves);
	}

	void generate_captures(const Position::Position& pos, MoveList& __restrict moves) {
		generate_moves<MoveType::CAPTURE>(pos, moves);
	}

	void generate_captures_and_promos(const Position::Position& pos, MoveList& __restrict moves) {
		generate_moves<MoveType::CAPTURE>(pos, moves);
		generate_quiet_promotions(pos, moves);
	}
	void generate_quiets(const Position::Position & pos, MoveList& __restrict moves) {
		generate_moves<MoveType::QUIET>(pos, moves);
	}

	void generate_all_moves(const Position::Position & pos, MoveList& __restrict moves) {
		generate_moves<MoveType::ALL>(pos, moves);
	}

	bool has_legal_moves(Position::Position& pos) {
		MoveList captures{};
		MoveGen::generate_captures(pos, captures);
		for (auto move : captures) {
			if (pos.is_legal(move)) return true;
		}

		MoveList quiets{};
		MoveGen::generate_quiets(pos, quiets);
		for (auto move : quiets) {
			if (pos.is_legal(move)) return true;
		}

		return false;
	}
}