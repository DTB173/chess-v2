module;
#include <array>
#include <iostream>
#include <random>
#include <bit>
export module AttackTables;
import Types;
import Magics;

export namespace AttackTables {
	using Types::ui64;
	using Types::Color;

	constexpr ui64 NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
	constexpr ui64 NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;

	consteval ui64 knight_attacks(ui64 b) {
		constexpr ui64 NOT_AB_FILE = 0xFCFCFCFCFCFCFCFCULL;
		constexpr ui64 NOT_GH_FILE = 0x3F3F3F3F3F3F3F3FULL;

		ui64 moves{};

		moves |= (b << 17) & NOT_A_FILE;  // 2 up,   1 right
		moves |= (b << 10) & NOT_AB_FILE; // 1 up,   2 right
		moves |= (b >> 6)  & NOT_AB_FILE; // 1 down, 2 right
		moves |= (b >> 15) & NOT_A_FILE;  // 2 down, 1 right

		moves |= (b << 15) & NOT_H_FILE;  // 2 up,   1 left
		moves |= (b << 6)  & NOT_GH_FILE; // 1 up,   2 left
		moves |= (b >> 10) & NOT_GH_FILE; // 1 down, 2 left
		moves |= (b >> 17) & NOT_H_FILE;  // 2 down, 1 left

		return moves;
	}

	consteval ui64 king_attacks(ui64 idx) {
		ui64 moves{};

		moves |= (idx << 8) | (idx >> 8); // Up, Down
		moves |= (idx & NOT_H_FILE) << 1; // Right
		moves |= (idx & NOT_A_FILE) >> 1; // Left
		moves |= (idx << 9) & NOT_A_FILE; // Up Left
		moves |= (idx << 7) & NOT_H_FILE; // Up Right
		moves |= (idx >> 9) & NOT_H_FILE; // Down Right
		moves |= (idx >> 7) & NOT_A_FILE; // Down Left

		return moves;
	}

	template<Color c>
	consteval ui64 pawn_attacks(ui64 idx) {
		ui64 moves{};
		if constexpr (c == Color::WHITE) {
			moves |= (idx << 7) & NOT_H_FILE; // Up Right
			moves |= (idx << 9) & NOT_A_FILE; // Up Left
		}
		else {
			moves |= (idx >> 7) & NOT_A_FILE; // Down Left
			moves |= (idx >> 9) & NOT_H_FILE; // Down Right
		}
		return moves;
	}

	constexpr ui64 queen_attacks_slow(ui64 idx, ui64 blockers) {
		return Magics::rook_attacks_slow(idx, blockers) | Magics::bishop_attacks_slow(idx, blockers);
	}

	consteval auto make_king_array() {
		std::array<ui64, 64> table;
		for (int shift{}; shift < 64; ++shift) {
			table[shift] = king_attacks(1ULL << shift);
		}
		return table;
	}

	consteval auto make_knight_array() {
		std::array<ui64, 64> table;
		for (int shift{}; shift < 64; ++shift) {
			table[shift] = knight_attacks(1ULL << shift);
		}
		return table;
	}

	consteval auto make_wpawn_array() {
		std::array<ui64, 64> table;
		for (int shift{}; shift < 64; ++shift) {
			table[shift] = pawn_attacks<Color::WHITE>(1ULL << shift);
		}
		return table;
	}

	consteval auto make_bpawn_array() {
		std::array<ui64, 64> table;
		for (int shift{}; shift < 64; ++shift) {
			table[shift] = pawn_attacks<Color::BLACK>(1ULL << shift);
		}
		return table;
	}

	constexpr inline auto knight_table     = make_knight_array();
	constexpr inline auto king_table       = make_king_array();
	constexpr inline auto white_pawn_table = make_wpawn_array();
	constexpr inline auto black_pawn_table = make_bpawn_array();

	constexpr ui64 get_knight_attacks(ui64 idx) { return knight_table[idx]; }
	constexpr ui64 get_king_attacks(ui64 idx) { return king_table[idx]; }
	constexpr ui64 get_pawn_attacks(ui64 idx, Color c) { return c == Color::WHITE ? white_pawn_table[idx] : black_pawn_table[idx]; }
	ui64 get_pawn_moves_and_attacks(int sq, Color us, ui64 total_occ, ui64 enemy_occ, int ep_file) {
		ui64 moves = 0;
		ui64 bit = (1ULL << sq);

		if (us == Color::WHITE) {
			// 1. Single Push: Shift up 8, mask with EMPTY squares
			ui64 push = (bit << 8) & ~total_occ;
			moves |= push;

			// 2. Double Push: Only from Rank 2, and only if first push was clear
			if (push && (sq >= 8 && sq <= 15)) {
				moves |= (push << 8) & ~total_occ;
			}
		}
		else {
			// 1. Single Push: Shift down 8, mask with EMPTY squares
			ui64 push = (bit >> 8) & ~total_occ;
			moves |= push;

			// 2. Double Push: Only from Rank 7
			if (push && (sq >= 48 && sq <= 55)) {
				moves |= (push >> 8) & ~total_occ;
			}
		}

		// 3. Normal Captures:
		moves |= (get_pawn_attacks(sq, us) & enemy_occ);

		// 4. En Passant: The "Magic" bit
		if (ep_file != -1) {
			// If the side to move has an EP opportunity, we add that square
			// White EP happens on Rank 6, Black on Rank 3
			int ep_rank = (us == Color::WHITE) ? 5 : 2;
			int ep_sq = ep_rank * 8 + ep_file;

			// Only add if the pawn at 'sq' is actually capable of hitting the ep_sq
			moves |= (get_pawn_attacks(sq, us) & (1ULL << ep_sq));
		}

		return moves;
	}
	inline uint64_t get_rook_attacks(int sq, uint64_t occupancy) {
		using namespace Magics;
		uint64_t relevant = occupancy & rook_masks[sq];
		uint64_t hash = relevant * rook_magics[sq];
		uint32_t index = rook_offsets[sq] + static_cast<uint32_t>(hash >> rook_shifts[sq]);
		return rook_attacks_table[index];
	}
	inline uint64_t get_bishop_attacks(int sq, uint64_t occupancy) {
		using namespace Magics;
		uint64_t relevant = occupancy & bishop_masks[sq];
		uint64_t hash = relevant * bishop_magics[sq];
		uint32_t index = bishop_offsets[sq] + static_cast<uint32_t>(hash >> bishop_shifts[sq]);
		return bishop_attacks_table[index];
	}
	inline uint64_t get_queen_attacks(int sq, uint64_t occupancy) {
		return get_rook_attacks(sq, occupancy) | get_bishop_attacks(sq, occupancy);
	}

	void verify_magic_attacks() {
		using namespace Magics;
		std::mt19937_64 rng(42);  // fixed seed for reproducibility

		for (int sq = 0; sq < 64; ++sq) {
			uint64_t mask = rook_masks[sq];

			// Test 1000 random blocker configurations per square
			for (int i = 0; i < 1000; ++i) {
				uint64_t blockers = 0;
				uint64_t temp = mask;
				while (temp) {
					int bit = std::countr_zero(temp);
					if (rng() & 1) blockers |= (1ULL << bit);
					temp &= temp - 1;
				}

				uint64_t magic_attacks = get_rook_attacks(sq, blockers);
				uint64_t slow_attacks = rook_attacks_slow(sq, blockers);

				if (magic_attacks != slow_attacks) {
					std::cerr << "VERIFICATION FAILED! Square " << sq << "\n";
					std::cerr << "Blockers: 0x" << std::hex << blockers << std::dec << "\n";
					std::cerr << "Magic: 0x" << std::hex << magic_attacks << "\n";
					std::cerr << "Slow:  0x" << slow_attacks << "\n";
					std::abort();
				}
			}

			// Also test empty occupancy
			if (get_rook_attacks(sq, 0) != rook_attacks_slow(sq, 0)) {
				std::cerr << "Failed on empty board, square " << sq << "\n";
				std::abort();
			}
		}

		std::cout << "All rook attacks verified.\n";
		std::cout << "rook: " << Magics::rook_table_size << '\n'
			<< "bishop: " << Magics::bishop_table_size << '\n';
	}

	void print_position(std::ostream& out, int idx) {
		ui64 moves = knight_table[idx];

		for (int r = 7; r >= 0; --r) { // Start from Rank 8
			for (int f = 0; f < 8; ++f) { // File A to H
				int sq = r * 8 + f;

				if (sq == idx) out << "* ";
				else out << ((moves & (1ULL << sq)) ? "1 " : ". ");
			}
			out << "\n";
		}
	}
	void print_entire_table(std::ostream& out) {
		out << '{';
		for (Types::ui32 i{}; i < 64; ++i) {
			out << Magics::bishop_offsets[i] << ", ";
		}
		out << '}';
	}
}

