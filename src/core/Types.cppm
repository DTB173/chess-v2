module;

#include <cstdint>
#include <string>

export module Types;

export namespace Types {
	using ui64 = std::uint64_t;
    using ui32 = std::uint32_t;
	using ui16 = std::uint16_t;
    using ui8  = std::uint8_t;
   
    namespace MoveFlags {
        // Basic types
        constexpr int Quiet      = 0b0000 << 12;
        constexpr int DoublePush = 0b0001 << 12;
        constexpr int EnPassant  = 0b0101 << 12; // Bit 14 set because it's a capture
        constexpr int Castling   = 0b0010 << 12;

        // Masks
        constexpr int Capture = 0b0100 << 12;
        constexpr int Promo   = 0b1000 << 12;

        // Specific Promotions
        constexpr int PromoKnight = 0b1000 << 12;
        constexpr int PromoBishop = 0b1001 << 12;
        constexpr int PromoRook   = 0b1010 << 12;
        constexpr int PromoQueen  = 0b1011 << 12;

        // Promotion + Capture (Promo bit + Capture bit + Piece bits)
        constexpr int PC_Knight = 0b1100 << 12;
        constexpr int PC_Bishop = 0b1101 << 12;
        constexpr int PC_Rook   = 0b1110 << 12;
        constexpr int PC_Queen  = 0b1111 << 12;
    }

    enum class Color : ui8 {
        NONE,
        WHITE,
        BLACK,
    };

    enum class PieceType : ui8 {
        NONE,
        PAWN,
        ROOK,
        KNIGHT,
        BISHOP,
        QUEEN,
        KING,
    };

    inline Color opponent_of(Color c) {
        return (c == Color::WHITE) ? Color::BLACK : Color::WHITE;
	}

    struct Score {
        int mg;
        int eg;

        void operator+=(const Score& other) { mg += other.mg; eg += other.eg; }
        void operator-=(const Score& other) { mg -= other.mg; eg -= other.eg; }
        Score operator*(int factor) { return { mg * factor, eg * factor }; }
    };

    struct Piece {
        ui8 data;

        static constexpr inline ui8 TYPE_MASK  = 0x07;
        static constexpr inline ui8 COLOR_MASK = 0x18;
        constexpr Piece() : data(0) {}

        constexpr Piece(Color c, PieceType t)
            : data((static_cast<ui8>(c) << 3) | static_cast<ui8>(t)) {
        }

        constexpr PieceType type() const { return static_cast<PieceType>(data & TYPE_MASK); }
        constexpr Color color()    const { return static_cast<Color>((data & COLOR_MASK) >> 3); }
        constexpr bool is_white()  const { return color() == Color::WHITE; }
        constexpr bool is_none()   const { return data == 0; }
    };

    struct Move {
        ui16 data;

        // Bit layout: 
        //  0- 5: From square
        //  6-11: To square
        // 12-15: Flags
        static constexpr ui16 FROM_MASK = 0x3F;
        static constexpr ui16 TO_MASK   = 0xFC0; // (0x3F << 6)
        static constexpr ui16 FLAG_MASK = 0xF000;

        constexpr Move() : data(0) {}

        constexpr Move(int from, int to, int flags = 0) {
            // flags should already be shifted (e.g., MoveFlags::EnPassant)
            data = static_cast<ui16>(from | (to << 6) | flags);
        }

        constexpr int from()  const { return data & FROM_MASK; }
        constexpr int to()    const { return (data & TO_MASK) >> 6; }
        constexpr int flags() const { return data & FLAG_MASK; }

        constexpr bool is_tactical()   const { return flags() & (MoveFlags::Promo | MoveFlags::Capture); }
        constexpr bool is_capture()    const { return flags() & MoveFlags::Capture; }
        constexpr bool is_promo()      const { return flags() & MoveFlags::Promo; }

        constexpr bool is_en_passant() const { return flags() == MoveFlags::EnPassant; }
        constexpr bool is_castling()   const { return flags() == MoveFlags::Castling; }

        Types::PieceType promotion_type() const {
            // Extract the 2 bits of the promotion (00: N, 01: B, 10: R, 11: Q)
            static constexpr Types::PieceType table[] = {
                PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN
            };
            return table[(data >> 12) & 0x3];
        }

        bool operator==(const Move& other) const { return data == other.data; }
    };

	constexpr Move NO_MOVE = Move();

    struct Square {
        enum SquareName : ui16 {
            SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
            SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
            SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
            SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
            SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
            SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
            SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
            SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,

            SQ_NONE
        };

        SquareName val{};

		constexpr Square() : val(SQ_NONE) {}
        constexpr Square(SquareName s) : val(s) {}
        constexpr Square(int s) : val(static_cast<SquareName>(s)) {}

        static constexpr Square from_rf(int rank, int file) {
            return Square((rank << 3) | (file & 7));
        }

        constexpr int file() const { return val & 7; }
        constexpr int rank() const { return val >> 3; }

        constexpr Square flipped() const { return Square(val ^ 56); }

        constexpr operator int() const { return static_cast<int>(val); }
    };

    constexpr char file_char(int sq) {
        return 'a' + (sq & 7);       // file = sq % 8
    }

    constexpr char rank_char(int sq) {
        return '1' + (sq >> 3);      // rank = sq / 8
    }

    std::string move_to_string(const Move& move) {
        int from_sq = move.from();
        int to_sq = move.to();

        std::string s;
        s += file_char(from_sq);
        s += rank_char(from_sq);
        s += file_char(to_sq);
        s += rank_char(to_sq);

        if (move.is_promo()) {
            s+= move.promotion_type() == PieceType::KNIGHT ? 'n' :
                move.promotion_type() == PieceType::BISHOP ? 'b' :
                move.promotion_type() == PieceType::ROOK   ? 'r' :
				'q';
        }

        return s;
    }
}
