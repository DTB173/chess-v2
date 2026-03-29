#pragma once

#include <array>
#include <iostream>

#include "Types.h"

namespace PSQTables{
	constexpr inline Types::Score MATERIAL_VALUES[6] = {
		{ 150,  191}, // PAWN
		{1044,  770}, // ROOK
		{ 723,  394}, // KNIGHT
		{ 759,  419}, // BISHOP
		{2172, 1444}, // QUEEN
		{20000, 20000}, // KING
	};

    constexpr inline std::array<int, 64> PAWN_PSQ_MG = {
           0,    0,    0,    0,    0,    0,    0,    0, // Rank 1
         -60,   11,  -18,  -27,    2,   55,  103,   -2, // Rank 2
         -55,   -8,    3,   -7,   25,  -12,   73,   -3, // Rank 3
         -67,   -3,   -8,   27,   25,    9,   17,  -61, // Rank 4
         -32,    6,   -2,   18,   43,   14,   18,  -39, // Rank 5
         -49,   -8,   29,   17,   55,  112,   11,  -32, // Rank 6
          90,   66,   50,  100,  142,  121,    5,  -67, // Rank 7
           0,    0,    0,    0,    0,    0,    0,    0, // Rank 8
    };

    constexpr inline std::array<int, 64> PAWN_PSQ_EG = {
           0,    0,    0,    0,    0,    0,    0,    0, // Rank 1
           2,  -14,  -12,  -10,  -27,  -31,  -44,  -44, // Rank 2
         -16,  -21,  -47,  -27,  -39,  -37,  -43,  -45, // Rank 3
          -2,  -18,  -44,  -51,  -52,  -52,  -38,  -33, // Rank 4
          32,   12,   -9,  -28,  -46,  -31,  -10,   -4, // Rank 5
         148,  140,  116,   86,   57,   41,   98,  111, // Rank 6
         263,  264,  234,  176,  155,  180,  254,  250, // Rank 7
           0,    0,    0,    0,    0,    0,    0,    0, // Rank 8
    };

    constexpr inline std::array<int, 64> ROOK_PSQ_MG = {
         -24,  -18,   10,   14,   21,   -2,  -36,   -5, // Rank 1
         -69,  -27,  -30,  -16,  -10,   -3,   27,  -80, // Rank 2
         -56,  -32,   -8,    9,    5,   -9,   46,   17, // Rank 3
         -50,  -49,  -42,  -11,    0,   -9,   22,   -2, // Rank 4
         -45,   -7,   36,   57,   20,   40,   81,   74, // Rank 5
           8,   23,   27,   72,  103,  127,  160,  109, // Rank 6
          24,    3,   73,  136,   73,  164,  180,  126, // Rank 7
          86,  114,  135,  142,  201,  254,  171,   66, // Rank 8
    };

    constexpr inline std::array<int, 64> ROOK_PSQ_EG = {
           2,   16,   13,   28,    2,   -2,    6,  -52, // Rank 1
           9,    4,   17,   13,    4,    1,  -11,    3, // Rank 2
           0,    6,    3,    0,    1,   -7,  -32,  -31, // Rank 3
          14,   19,   25,   21,   12,    4,  -14,  -22, // Rank 4
          23,   12,   13,    4,    8,    2,  -16,  -21, // Rank 5
          19,   17,   16,    2,  -11,  -15,  -21,  -21, // Rank 6
          20,   43,   23,    1,    3,  -11,  -22,  -22, // Rank 7
          23,    9,    8,    0,  -18,  -39,  -24,    4, // Rank 8
    };

    constexpr inline std::array<int, 64> KNIGHT_PSQ_MG = {
         -80,   -8,  -62,  -26,  -26,   -1,  -14, -114, // Rank 1
         -16,  -26,   19,   32,   32,   51,   20,   10, // Rank 2
          -6,   19,   59,   49,   66,   48,   65,  -12, // Rank 3
           7,   27,   55,   50,   69,   71,   38,  -11, // Rank 4
          48,   54,   52,  119,   66,  108,   33,   71, // Rank 5
          14,   64,   70,  141,  191,  235,  140,   12, // Rank 6
         -98,  -26,  145,   61,  165,  149,   17,   14, // Rank 7
        -431, -105,  -58, -101,   75, -204, -254, -240, // Rank 8
    };

    constexpr inline std::array<int, 64> KNIGHT_PSQ_EG = {
         -86,  -64,  -21,   -8,  -23,  -11,  -65,  -48, // Rank 1
         -47,  -30,  -19,    0,   -4,  -12,  -34,  -47, // Rank 2
         -33,    2,   -5,   33,   28,    4,  -19,  -45, // Rank 3
         -17,    6,   38,   41,   45,   17,   -7,   -8, // Rank 4
         -22,   24,   33,   45,   41,   28,   20,  -30, // Rank 5
         -40,  -23,   27,   17,  -26,  -19,  -39,  -30, // Rank 6
         -43,  -21,  -40,  -12,  -51,  -50,  -46,  -81, // Rank 7
          -7,  -75,  -30,  -13,  -55,  -27,  -26, -130, // Rank 8
    };

    constexpr inline std::array<int, 64> BISHOP_PSQ_MG = {
          67,   18,   52,   29,   17,   21,   56,   40, // Rank 1
          31,  107,   69,   64,   66,  106,  123,   70, // Rank 2
          94,   87,   89,   76,   87,   89,   76,   83, // Rank 3
          33,   71,   71,  110,  112,   78,   69,   12, // Rank 4
           7,   51,   91,  101,  104,   61,   72,   39, // Rank 5
          28,   66,  135,   91,  161,  143,  131,  118, // Rank 6
           7,   75,   34,   43,   78,  156,   59,   89, // Rank 7
         -20,  -20,   14,  -51, -154,  -23,   62,  -83, // Rank 8
    };

    constexpr inline std::array<int, 64> BISHOP_PSQ_EG = {
         -49,  -13,  -45,    0,   -3,  -19,  -39,  -30, // Rank 1
          -9,  -32,  -13,    6,   12,  -10,  -18,  -39, // Rank 2
         -29,    3,   13,   25,   28,    9,  -17,  -30, // Rank 3
          -7,    5,   28,   16,   19,   27,   -6,  -13, // Rank 4
          17,   28,   17,   31,   27,   12,    5,  -10, // Rank 5
           7,    8,    5,   24,   -5,   17,   -1,  -19, // Rank 6
         -22,   -2,    4,   -9,    1,  -26,    2,  -59, // Rank 7
          -6,  -28,  -24,    7,   16,   -1,  -30,    9, // Rank 8
    };

    constexpr inline std::array<int, 64> QUEEN_PSQ_MG = {
          35,    8,   25,   56,   17,   -5,  -56,   19, // Rank 1
           0,   42,   47,   44,   52,   80,   86,   52, // Rank 2
           8,   44,   26,   36,   31,   44,   48,   26, // Rank 3
           5,   17,   14,   21,   37,   33,   44,   16, // Rank 4
         -19,    6,   18,   14,   20,   35,   11,   58, // Rank 5
          -5,    6,   40,   77,   94,  175,  233,  140, // Rank 6
          -9,  -25,  -23,  -71,  -39,  205,   56,  260, // Rank 7
         -64,    5,   79,  148,  184,  183,  -18,  -56, // Rank 8
    };

    constexpr inline std::array<int, 64> QUEEN_PSQ_EG = {
         -59,  -66,  -64, -133,  -39,  -70,  -44, -109, // Rank 1
         -30,  -29,  -48,  -16,  -35, -104, -120,  -91, // Rank 2
         -71,  -81,   21,  -15,   17,    2,    7,  -41, // Rank 3
         -27,    3,   11,   62,   38,   19,   14,  -17, // Rank 4
         -27,   19,   14,   64,   97,   90,   62,    1, // Rank 5
         -47,   -8,   38,   11,   87,  -11,  -69,  -62, // Rank 6
          -8,    4,   48,  130,  101,   39,   61, -167, // Rank 7
          16,   11,   -6,  -13,  -44,  -35,    5,   72, // Rank 8
    };

    constexpr inline std::array<int, 64> KING_PSQ_MG = {
        -144,   -9,  -58, -183,  -64, -127,    9,   -1, // Rank 1
          16,  -74, -109, -203, -176, -119,  -15,  -15, // Rank 2
         -24,  -11, -168, -193, -197, -154,  -70, -112, // Rank 3
          33,    8,  -28, -157, -140, -172, -107, -190, // Rank 4
         -18,  -16,   39,  -46,  -72,  -54,  -24, -116, // Rank 5
          54,  218,   31,   79,  -17,   91,  114,  -66, // Rank 6
          56,  187,  107,   66,   73,  180,   -8,  -85, // Rank 7
         190,  102,  144,  123,  154,  128,   67,   73, // Rank 8
    };

    constexpr inline std::array<int, 64> KING_PSQ_EG = {
         -53,  -55,  -29,   -8,  -50,  -15,  -65, -105, // Rank 1
         -62,  -12,   17,   44,   42,   31,   -6,  -37, // Rank 2
         -60,  -16,   31,   50,   55,   45,   13,   -6, // Rank 3
         -71,  -20,   14,   44,   48,   53,   23,    6, // Rank 4
         -51,   -8,    5,   25,   27,   38,   26,   11, // Rank 5
         -40,  -20,    5,   -2,   17,   24,   31,   21, // Rank 6
         -48,  -24,  -19,  -16,   -9,    6,   31,   14, // Rank 7
        -126,  -58,  -62,  -51,  -47,  -28,  -27,  -74, // Rank 8
    };

	consteval std::array<Types::Score, 64> create_array(const std::array<int, 64>& mg_table, 
												        const std::array<int, 64>& eg_table) 
	{
		std::array<Types::Score, 64> result{};
		for (int i = 0; i < 64; ++i) {
			result[i] = Types::Score{ mg_table[i], eg_table[i] };
		}
		return result;
	}

	// Build individual piece PSQ tables
	constexpr std::array<Types::Score, 64> PAWN_PSQ = create_array(PAWN_PSQ_MG, PAWN_PSQ_EG);
	constexpr std::array<Types::Score, 64> BISHOP_PSQ = create_array(BISHOP_PSQ_MG, BISHOP_PSQ_EG);
	constexpr std::array<Types::Score, 64> ROOK_PSQ = create_array(ROOK_PSQ_MG, ROOK_PSQ_EG);
	constexpr std::array<Types::Score, 64> KNIGHT_PSQ_TABLE = create_array(KNIGHT_PSQ_MG, KNIGHT_PSQ_EG);
	constexpr std::array<Types::Score, 64> QUEEN_PSQ_TABLE = create_array(QUEEN_PSQ_MG, QUEEN_PSQ_EG);
	constexpr std::array<Types::Score, 64> KING_PSQ = create_array(KING_PSQ_MG, KING_PSQ_EG);

	// Aggregate all PSQ tables
	constexpr const std::array<std::array<Types::Score, 64>, 6> PSQ_TABLES = {
		PAWN_PSQ,
		ROOK_PSQ,
		KNIGHT_PSQ_TABLE,
		BISHOP_PSQ,
		QUEEN_PSQ_TABLE,
		KING_PSQ
	};

	constexpr inline Types::Score get_material_value(Types::PieceType type) {
		return MATERIAL_VALUES[static_cast<int>(type) - 1];
	}

	constexpr inline const Types::Score get_piece_value(Types::PieceType type, Types::Color color, int square) {
		int index = (color == Types::Color::WHITE) ? square : (square ^ 56);
		Types::Score s = PSQ_TABLES[static_cast<int>(type) - 1][index];
		Types::Score mat = get_material_value(type);

		return s + mat;
	}

	// Debug PSQT print
	void print_position() {
		for (int r = 7; r >= 0; --r) { // Start from Rank 8
			for (int f = 0; f < 8; ++f) { // File A to H
				int sq = r * 8 + f;
				std::cout << PAWN_PSQ_MG[sq] << ' ';
			}
			std::cout << "\n";
		}
	}
}