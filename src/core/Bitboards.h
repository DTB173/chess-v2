#pragma once

#include <iostream>
#include <immintrin.h>
#include <bit>
#include <array>

#include "Types.h"

namespace Bitwise{
	template<std::integral T>
	constexpr void set_bit(T& bits, int index) { bits |= 1ULL << index; }
	
	template<std::integral T>
	constexpr bool test_bit(T bits, int index) { return bits & 1ULL << index; }
	template<std::integral T>
	constexpr void clear_bit(T& bits, int index) { bits &= ~(1ULL << index); }

	template<std::integral T>
	inline int pop_lsb(T& bits) {
		int s = _tzcnt_u64(bits); 
		bits = _blsr_u64(bits); 
		return s;
	}

	template<std::integral T>
	constexpr int lsb(T bits) {
		assert(bits != 0);
		return std::countr_zero(static_cast<unsigned long long>(bits));
	}

	template<std::integral T>
	constexpr int msb(T bits) {
		assert(bits != 0);
		return 63 - std::countl_zero(static_cast<unsigned long long>(bits));
	}

	template<std::integral T>
	constexpr int count(T bits) {
#if defined(_MSC_VER) && defined(__AVX2__)
		return static_cast<int>(__popcnt64(bits));
#else
		return std::popcount(bits);
#endif
	}
}

namespace Bitboards {
	using namespace Types;

	constexpr ui64 FILE_MASKS[8] = {
		0x0101010101010101ULL, // A
		0x0202020202020202ULL, // B
		0x0404040404040404ULL, // C
		0x0808080808080808ULL, // D
		0x1010101010101010ULL, // E
		0x2020202020202020ULL, // F
		0x4040404040404040ULL, // G
		0x8080808080808080ULL  // H
	};

	constexpr ui64 RANK_MASKS[8] = {
		0x00000000000000FFULL, // 1
		0x000000000000FF00ULL, // 2
		0x0000000000FF0000ULL, // 3
		0x00000000FF000000ULL, // 4
		0x000000FF00000000ULL, // 5
		0x0000FF0000000000ULL, // 6
		0x00FF000000000000ULL, // 7
		0xFF00000000000000ULL  // 8
	};

	constexpr ui64 LONG_DIAGONALS = 0x8142241818244281ull;
	constexpr ui64 CENTER_SQUARES = 0x0000001818000000ull;



	constexpr inline ui64 rank_mask(int rank) {
		return RANK_MASKS[rank];
	}

	constexpr inline ui64 file_mask(int file) {
		return FILE_MASKS[file];
	}

	constexpr inline int rank_of(Square sq) {
		return sq / 8;
	}

	constexpr inline int file_of(Square sq) {
		return sq % 8;
	}

	inline int distance(Square s1, Square s2) {
		int dx = std::abs(file_of(s1) - file_of(s2));
		int dy = std::abs(rank_of(s1) - rank_of(s2));
		return std::max(dx, dy);
	}

	constexpr int mirror_file(int file) {
		constexpr int Mirror[] = { 0,1,2,3,3,2,1,0 };
		return Mirror[file];
	}

	constexpr inline int relative_rank_of(Square sq, Color c) {
		return c == Color::WHITE ? rank_of(sq) : 7 - rank_of(sq);
	}

	constexpr inline int relative_square32(Square sq, Color c) {
		return 4 * relative_rank_of(sq, c) + mirror_file(file_of(sq));
	}

	inline Square backmost(Color c, ui64 pawns) {
		return c == Color::WHITE ? Bitwise::lsb(pawns) : Bitwise::msb(pawns);
	}

	inline Square frontmost(Color c, ui64 pawns) {
		return c == Color::WHITE ? Bitwise::msb(pawns) : Bitwise::lsb(pawns);
	}


	constexpr inline std::array<ui64, 2> outpost_ranks_masks = {
		RANK_MASKS[3] | RANK_MASKS[4] | RANK_MASKS[5],
		RANK_MASKS[2] | RANK_MASKS[3] | RANK_MASKS[4]
	};

	constexpr inline ui64 get_outpost_rank_mask(Color c) {
		return outpost_ranks_masks[static_cast<int>(c) - 1];
	}

	constexpr inline bool several(ui64 b) {
		return b & (b - 1);
	}

	consteval std::array<ui64, 8> init_rank_below() {
		std::array<ui64, 8> t{};
		ui64 current_below = 0;
		for (int r = 0; r < 8; ++r) {
			t[r] = current_below;
			current_below |= RANK_MASKS[r];
		}
		return t;
	}
	consteval std::array<ui64, 8> init_rank_above() {
		std::array<ui64, 8> t{};
		ui64 current_above = 0;
		for (int r = 7; r >= 0; --r) {
			t[r] = current_above;
			current_above |= RANK_MASKS[r];
		}
		return t;
	}

	constexpr inline ui64 file_fill(int file) {
		ui64 mask = FILE_MASKS[file];
		if (file > 0) mask |= FILE_MASKS[file - 1];
		if (file < 7) mask |= FILE_MASKS[file + 1];
		return mask;
	}

	constexpr ui64 make_passed_pawn_mask(int idx, Color side) {
		int file = idx % 8;
		int rank = idx / 8;
		ui64 mask = 0;
		ui64 adjacent_files = file_fill(file);

		if (side == Color::WHITE) {
			for (int r = rank + 1; r < 8; ++r) {
				mask |= (adjacent_files & RANK_MASKS[r]);
			}
		}
		else {
			for (int r = rank - 1; r >= 0; --r) {
				mask |= (adjacent_files & RANK_MASKS[r]);
			}
		}
		return mask;
	}

	consteval std::array<std::array<ui64, 64>, 2> init_passed_masks() {
		std::array<std::array<ui64, 64>, 2> mask{};

		for (Color c : {Color::WHITE, Color::BLACK}) {
			for (int sq = 0; sq < 64; ++sq) {
				mask[static_cast<int>(c) - 1][sq] = make_passed_pawn_mask(sq, c);
			}
		}
		return mask;
	}

	consteval std::array<std::array<ui64, 64>, 2> init_forward_file_masks() {
		std::array<std::array<ui64, 64>, 2> forward_mask{};
		for (int sq = 0; sq < 64; ++sq) {
			ui64 mask_white = 0;
			for (int r = rank_of(sq) + 1; r < 8; ++r) {
				mask_white |= (1ULL << Square::from_rf(r, file_of(sq)));
			}
			forward_mask[0][sq] = mask_white;

			ui64 mask_black = 0;
			for (int r = rank_of(sq) - 1; r >= 0; --r) {
				mask_black |= (1ULL << Square::from_rf(r, file_of(sq)));
			}
			forward_mask[1][sq] = mask_black;
		}
		return forward_mask;
	}

	constexpr inline auto PASSED_MASK = init_passed_masks();

	consteval std::array<std::array<ui64, 64>, 2> init_outpost_square_masks() {
		std::array<std::array<ui64, 64>, 2> masks{};

		for (int color = 0; color < 2; ++color) {
			for (int sq = 0; sq < 64; ++sq) {
				masks[color][sq] = PASSED_MASK[color][sq] & ~FILE_MASKS[file_of(sq)];
			}
		}
		return masks;
	}

	constexpr inline auto OUTPOST_SQUARE_MASK = init_outpost_square_masks();

	constexpr inline ui64 outpost_square_mask(Square sq, Color c) {
		return OUTPOST_SQUARE_MASK[static_cast<int>(c) - 1][sq];
	}

	constexpr inline auto RANKS_BELOW = init_rank_below();
	constexpr inline auto RANKS_ABOVE = init_rank_above();

	constexpr inline auto FORWARD_FILE_MASK = init_forward_file_masks();

	consteval std::array<ui64, 64> init_king_zone() {
		std::array<ui64, 64> zones{};

		for (int sq = 0; sq < 64; ++sq) {
			ui64 zone = 0;

			int king_file = file_of(sq);
			int king_rank = rank_of(sq);

			for (int f = king_file - 1; f <= king_file + 1; ++f) {
				if (f < 0 || f > 7) continue;

				for (int r = 0; r < 4; ++r) { 
					int rank = (king_rank < 4) ? king_rank + r
						: king_rank - r;

					if (rank >= 0 && rank < 8) {
						zone |= (1ULL << Square::from_rf(rank, f));
					}
				}
			}

			zone |= (1ULL << sq);

			zones[sq] = zone;
		}
		return zones;
	}

	constexpr inline auto KING_ZONE = init_king_zone();
	constexpr inline ui64 king_area(Square sq) { return KING_ZONE[sq]; }
	constexpr inline ui64 forward_file_mask(Square sq, Color c) {
		return FORWARD_FILE_MASK[static_cast<int>(c) - 1][sq];
	}

	constexpr inline ui64 adjacent_files_mask(int file) {
		if (file == 0) return FILE_MASKS[1];
		else if (file == 7) return FILE_MASKS[6];
		else return FILE_MASKS[file - 1] | FILE_MASKS[file + 1];
	}

	ui64 get_rank_mask_behind(int sq, Color side) {
		return side == Color::WHITE ?
			RANKS_BELOW[sq / 8] : RANKS_ABOVE[sq / 8];
	}

	ui64 get_passed_mask(Square sq, Color side) {
		return PASSED_MASK[static_cast<int>(side)-1][sq];
	}
}
