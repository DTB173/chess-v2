module;
#include <iostream>
#include <immintrin.h>
#include <bit>
#include <array>
export module Bitboards;
import Types;

export namespace Bitwise{
	template<std::integral T>
	constexpr void set_bit(T& bits, int index) { bits |= 1ULL << index; }
	
	template<std::integral T>
	constexpr void clear_bit(T& bits, int index) { bits &= ~(1ULL << index); }

	template<std::integral T>
	inline int pop_lsb(T& bits) {
		int s = _tzcnt_u64(bits);    // Direct hardware call
		bits = _blsr_u64(bits);      // BMI1 hardware bit reset
		return s;
	}

	template<std::integral T>
	constexpr int lsb(T bits) {
		return static_cast<int>(_tzcnt_u64(bits));
	}

	template<std::integral T>
	constexpr int count(T bits) {
#if defined(_MSC_VER) && defined(__AVX2__)
		return static_cast<int>(__popcnt64(bits));
#else
		return std::popcount(bits); // Clean fallback
#endif
	}
}
export namespace Bitboards {
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

	constexpr inline auto RANKS_BELOW = init_rank_below();
	constexpr inline auto RANKS_ABOVE = init_rank_above();

	constexpr inline ui64 rank_mask(int rank) {
		return RANK_MASKS[rank];
	}

	constexpr inline ui64 file_mask(int file) {
		return FILE_MASKS[file];
	}
	constexpr inline ui64 adjacent_files_mask(int file) {
		if (file == 0) return FILE_MASKS[1];
		else if (file == 7) return FILE_MASKS[6];
		else return FILE_MASKS[file - 1] | FILE_MASKS[file + 1];
	}

	constexpr inline ui64 file_fill(int file) {
		ui64 mask = FILE_MASKS[file];
		if (file > 0) mask |= FILE_MASKS[file - 1];
		if (file < 7) mask |= FILE_MASKS[file + 1];
		return mask;
	}


	ui64 get_passed_pawn_mask(int idx, Color side) {
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

	ui64 get_rank_mask_behind(int sq, Color side) {
		int rank = sq / 8;
		if (side == Color::WHITE) {
			return RANKS_BELOW[rank];
		}
		else {
			return RANKS_ABOVE[rank];
		}
	}


}
