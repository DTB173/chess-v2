module;
#include <array>

export module PSQTables;
import Types;

export namespace PSQTables{

	constexpr std::array<int ,6> MATERIAL_VALUES = {
		100,  // PAWN
		500,  // ROOK
		320,  // KNIGHT
		330,  // BISHOP
		900,  // QUEEN
		20000 // KING (not really used in eval)
	};

	constexpr std::array<int, 64> PAWN_PSQ_MG = {
		 0,  0,  0,  0,  0,  0,  0,  0,       
		 5, 10, 10,-20,-20, 10, 10,  5,      
		 5, -5,-10,  0,  0,-10, -5,  5,      
		 0,  0,  0, 20, 20,  0,  0,  0,      
		 5,  5, 10, 25, 25, 10,  5,  5,      
		10, 10, 20, 30, 30, 20, 10, 10,       
		50, 50, 50, 50, 50, 50, 50, 50,       
		 0,  0,  0,  0,  0,  0,  0,  0        
	};
	constexpr std::array<int, 64> PAWN_PSQ_EG = {
		0,  0,  0,  0,  0,  0,  0,  0,
		50, 50, 50, 50, 50, 50, 50, 50,
		30, 30, 30, 30, 30, 30, 30, 30,
		20, 20, 20, 20, 20, 20, 20, 20,
		10, 10, 10, 10, 10, 10, 10, 10,
		5,  5,  5,  5,  5,  5,  5,  5,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	};

	constexpr std::array<int, 64> KNIGHT_PSQ = {
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50
	};

	constexpr std::array<int, 64> BISHOP_PSQ_MG = {
		-20,-10,-10,-10,-10,-10,-10,-20,     
		-10,  0,  0,  0,  0,  0,  0,-10,     
		-10,  0,  5, 10, 10,  5,  0,-10,     
		-10,  5,  5, 10, 10,  5,  5,-10,      
		-10,  0, 10, 10, 10, 10,  0,-10,   
		-10, 10, 10, 10, 10, 10, 10,-10,     
		-10,  5,  0,  0,  0,  0,  5,-10,      
		-20,-10,-10,-10,-10,-10,-10,-20
	};
	constexpr std::array<int, 64> BISHOP_PSQ_EG = {
		-10, -5, -5, -5, -5, -5, -5,-10,
		-5,   0,  0,  0,  0,  0,  0, -5,
		-5,   0,  5, 10, 10,  5,  0, -5,
		-5,   5,  5, 10, 10,  5,  5, -5,
		-5,   0, 10, 10, 10, 10,  0, -5,
		-5,  10, 10, 10, 10, 10, 10, -5,
		-5,   5,  0,  0,  0,  0,  5, -5,
	   -10,  -5, -5, -5, -5, -5, -5,-10
	};

	constexpr std::array<int, 64> ROOK_PSQ_MG = {
		 0,  0,  0,  5,  5,  0,  0,  0,      
		 5, 10, 10, 10, 10, 10, 10,  5,     
		-5,  0,  0,  0,  0,  0,  0, -5,      
		-5,  0,  0,  0,  0,  0,  0, -5,      
		-5,  0,  0,  0,  0,  0,  0, -5,      
		-5,  0,  0,  0,  0,  0,  0, -5,      
		-5,  0,  0,  0,  0,  0,  0, -5,      
		 0,  0,  0,  5,  5,  0,  0,  0       
	};
	constexpr std::array<int, 64> ROOK_PSQ_EG = {
		 0,  0,  0,  0,  0,  0,  0,  0,
		20, 20, 20, 20, 20, 20, 20, 20,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  5,  5,  0,  0,  0
	};

	constexpr std::array<int, 64> QUEEN_PSQ = {
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20
	};

	constexpr std::array<int, 64> KING_PSQ_MG = {
		-30,-40,-40,-50,-50,-40,-40,-30,     
		-30,-40,-40,-50,-50,-40,-40,-30,     
		-30,-40,-40,-50,-50,-40,-40,-30,     
		-30,-40,-40,-50,-50,-40,-40,-30,     
		-20,-30,-30,-40,-40,-30,-30,-20,     
		-10,-20,-20,-20,-20,-20,-20,-10,     
		 20, 20,  0,  0,  0,  0, 20, 20,     
		 20, 30, 10,  0,  0, 10, 30, 20 
	};
	constexpr std::array<int, 64> KING_PSQ_EG = {
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  0,  0,  0,  0,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50
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

	constexpr std::array<Types::Score, 64> PAWN_PSQ = create_array(PAWN_PSQ_MG, PAWN_PSQ_EG);
	constexpr std::array<Types::Score, 64> BISHOP_PSQ = create_array(BISHOP_PSQ_MG, BISHOP_PSQ_EG);
	constexpr std::array<Types::Score, 64> ROOK_PSQ = create_array(ROOK_PSQ_MG, ROOK_PSQ_EG);
	constexpr std::array<Types::Score, 64> KNIGHT_PSQ_TABLE = create_array(KNIGHT_PSQ, KNIGHT_PSQ);
	constexpr std::array<Types::Score, 64> QUEEN_PSQ_TABLE = create_array(QUEEN_PSQ, QUEEN_PSQ);
	constexpr std::array<Types::Score, 64> KING_PSQ = create_array(KING_PSQ_MG, KING_PSQ_EG);

	constexpr const std::array<std::array<Types::Score, 64>, 6> PSQ_TABLES = {
		PAWN_PSQ,
		ROOK_PSQ,
		KNIGHT_PSQ_TABLE,
		BISHOP_PSQ,
		QUEEN_PSQ_TABLE,
		KING_PSQ
	};

	inline int get_material_value(Types::PieceType type) {
		return MATERIAL_VALUES[static_cast<int>(type) - 1];
	}

	inline const Types::Score get_piece_value(Types::PieceType type, Types::Color color, int square) {
		int index = (color == Types::Color::WHITE) ? square : (square ^ 56);
		Types::Score s = PSQ_TABLES[static_cast<int>(type) - 1][index];
		int mat = get_material_value(type);
		return {s.mg + mat, s.eg + mat};
	}

}