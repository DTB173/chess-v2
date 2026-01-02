module;
#include <array>
#include <vector>
#include <ostream>
#include <cassert>
export module Position;

import Types;
import Bitboards;
import AttackTables;

export namespace Position {
    using namespace Types;

    constexpr int bb_index(Piece p) {
        return (static_cast<int>(p.color()) - 1) * 6
             + (static_cast<int>(p.type()) - 1);
    }

    constexpr char piece_to_char(Piece p) {
        constexpr int offset = 'a' - 'A';
        constexpr char table[] = { '.','p','r','n','b','q','k' };

        static_assert(static_cast<int>(PieceType::NONE) == 0);
        return table[static_cast<int>(p.type())] - p.is_white() * offset;
    }

	// get king to sq, return rook from/to squares
    struct RookMove {
        Square from;
        Square to;
    };

    constexpr RookMove NO_ROOK_MOVE{ Square::SQ_NONE, Square::SQ_NONE };

    constexpr std::array<RookMove, 64> CASTLE_ROOK = []() constexpr{
        std::array<RookMove, 64> t{};

        t[Square::SQ_G1] = { Square::SQ_H1, Square::SQ_F1 };
        t[Square::SQ_C1] = { Square::SQ_A1, Square::SQ_D1 };
        t[Square::SQ_G8] = { Square::SQ_H8, Square::SQ_F8 };
        t[Square::SQ_C8] = { Square::SQ_A8, Square::SQ_D8 };

        return t;
    }();

    class Flags {
    private:
        ui16 bits{ 0xF };

        enum class MASKS : ui16 {
            WhiteKingside  = 1 << 0,
            WhiteQueenside = 1 << 1,
            BlackKingside  = 1 << 2,
            BlackQueenside = 1 << 3,
            EPFile         = 0b0111'0000,  // 3 bits for en passant file
            EPValid        = 1 << 7,
            BlackToMove    = 1 << 8        // 0 = white, 1 = black
        };

        // Helper to cast enum class to underlying type
        static constexpr ui16 mask(MASKS m) { return static_cast<ui16>(m); }

    public:
        // Castling queries
        bool can_white_kingside()  const { return bits & mask(MASKS::WhiteKingside); }
        bool can_white_queenside() const { return bits & mask(MASKS::WhiteQueenside); }
        bool can_black_kingside()  const { return bits & mask(MASKS::BlackKingside); }
        bool can_black_queenside() const { return bits & mask(MASKS::BlackQueenside); }

        // En passant
        int en_passant_file() const {
            return (bits & mask(MASKS::EPValid))
                   ? ((bits & mask(MASKS::EPFile)) >> 4)
                     : -1;
        }

        Square en_passant_square() const {
            int file = en_passant_file();
            if (file == -1) return Square::SQ_NONE;

            return (side_to_move() == Color::WHITE) ?
                static_cast<Square>(40 + file) :
                static_cast<Square>(16 + file);
        }

        void set_en_passant_file(int file) {
            clear_en_passant();
            if (file >= 0 && file < 8)
                bits |= (file << 4) | mask(MASKS::EPValid);
        }

        void clear_en_passant() {
            bits &= ~(mask(MASKS::EPFile) | mask(MASKS::EPValid));
        }

        void update_castling_rights(int from, int to) {
            constexpr ui16 CastlingRightsMask[64] = {
                13, 15, 15, 15, 12, 15, 15, 14, // Rank 1: A1=1101, E1=1100, H1=1110
                15, 15, 15, 15, 15, 15, 15, 15,
                15, 15, 15, 15, 15, 15, 15, 15,
                15, 15, 15, 15, 15, 15, 15, 15,
                15, 15, 15, 15, 15, 15, 15, 15,
                15, 15, 15, 15, 15, 15, 15, 15,
                15, 15, 15, 15, 15, 15, 15, 15,
                 7, 15, 15, 15,  3, 15, 15, 11  // Rank 8: A8=0111, E8=0011, H8=1011
            };
			ui64 final_mask = CastlingRightsMask[from] & CastlingRightsMask[to];
			bits &= (final_mask | 0xFFF0);
        }

        // Side to move
        Color side_to_move() const { return bits & mask(MASKS::BlackToMove) ? Color::BLACK : Color::WHITE; }
        void toggle_turn() { bits ^= mask(MASKS::BlackToMove); }
    };

	class Position {
	private:
        struct StateRecord {
            ui64 prev_zobrist;
            Move prev_half_move;
            Piece captured_piece;
            Flags prev_flags;
        };

        std::array<StateRecord, 1024> history;
        std::array<ui64, 12> board{};
        std::array<ui64, 2> occupancy{};
        ui64 total_pieces;

        std::array<Piece, 64> mailbox{};

        ui64 zobrist_key{};
        Flags metadata{};
        size_t history_idx{};
    public:
        const std::array<Piece, 64>& get_mailbox() const { return mailbox; }
        Flags get_metadata() const { return metadata; }
        inline Square get_king_square(Color c) const { 
            ui64 bb = board[bb_index(Piece(c, PieceType::KING))];
			return Bitwise::lsb(bb);;
        }
        void init_start_pos() {
            using Types::Square;
            using Types::Piece;

            // Clear all boards
            for (int i = 0; i < 12; ++i) board[i] = 0ULL;
            for (int i = 0; i < 64; ++i) mailbox[i] = Piece();

            auto place = [&](int sq, Piece piece) {
                int idx = bb_index(piece);
                Bitwise::set_bit(board[idx], sq);
                mailbox[sq] = piece;
                };

            // White pieces
            place(Square::SQ_A1, Piece(Color::WHITE, PieceType::ROOK));
            place(Square::SQ_B1, Piece(Color::WHITE, PieceType::KNIGHT));
            place(Square::SQ_C1, Piece(Color::WHITE, PieceType::BISHOP));
            place(Square::SQ_D1, Piece(Color::WHITE, PieceType::QUEEN));
            place(Square::SQ_E1, Piece(Color::WHITE, PieceType::KING));
            place(Square::SQ_F1, Piece(Color::WHITE, PieceType::BISHOP));
            place(Square::SQ_G1, Piece(Color::WHITE, PieceType::KNIGHT));
            place(Square::SQ_H1, Piece(Color::WHITE, PieceType::ROOK));

            for (int f = 0; f < 8; ++f)
                place(static_cast<Square>(f + 8), Piece(Color::WHITE, PieceType::PAWN)); // Rank 2

            // Black pieces
            place(Square::SQ_A8, Piece(Color::BLACK, PieceType::ROOK));
            place(Square::SQ_B8, Piece(Color::BLACK, PieceType::KNIGHT));
            place(Square::SQ_C8, Piece(Color::BLACK, PieceType::BISHOP));
            place(Square::SQ_D8, Piece(Color::BLACK, PieceType::QUEEN));
            place(Square::SQ_E8, Piece(Color::BLACK, PieceType::KING));
            place(Square::SQ_F8, Piece(Color::BLACK, PieceType::BISHOP));
            place(Square::SQ_G8, Piece(Color::BLACK, PieceType::KNIGHT));
            place(Square::SQ_H8, Piece(Color::BLACK, PieceType::ROOK));

            for (int f = 0; f < 8; ++f)
                place(static_cast<Square>(f + 48), Piece(Color::BLACK, PieceType::PAWN)); // Rank 7

            // Update occupancy
            update_occupancy();

        }

        void make_move(Move m) {
            int from = m.from();
            int to = m.to();
            int flags = m.flags();
            Color us = metadata.side_to_move();
            Piece moving_piece = mailbox[from];
            Piece captured = mailbox[to];

            // 1. History (Must store captured piece for undo)
            history[history_idx++] = { zobrist_key, m, captured, metadata };

            // 2. Handle Capture
            if (!captured.is_none() && flags != MoveFlags::EnPassant) {
                remove_piece(captured, to);
            }
            // 3. Basic Movement
            move_piece(moving_piece, from, to);

            // 4. Special Cases
			// Pawn Double Push, En Passant, Promotion
            metadata.clear_en_passant();
            metadata.update_castling_rights(from, to);
            if (m.is_promotion()) {
                remove_piece(moving_piece, to);
                place_piece(Piece(us, m.promotion_type()), to);
            }
            else if (flags == MoveFlags::DoublePush) {
                metadata.set_en_passant_file(Square(from).file());
            }
            else if (flags == MoveFlags::EnPassant) {
                int victim_sq = (us == Color::WHITE) ? to - 8 : to + 8;
                remove_piece(mailbox[victim_sq], victim_sq);
            }
            else if (flags == MoveFlags::Castling) {
				auto [r_from, r_to] = CASTLE_ROOK[to];
				move_piece(mailbox[r_from], r_from, r_to); // Rook goes from corner to landing
            }
            metadata.toggle_turn();
        }

        void undo_move() {
            StateRecord prev = history[--history_idx];
            const int from = prev.prev_half_move.from();
            const int to = prev.prev_half_move.to();
            const int flags = prev.prev_half_move.flags();
            const Color us = prev.prev_flags.side_to_move();
            const Color enemy = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;

            Piece moving_piece = mailbox[to];

            // 1. Restore the piece that moved
            if (prev.prev_half_move.is_promotion()) {
                remove_piece(moving_piece, static_cast<Square>(to));
                place_piece(Piece(us, PieceType::PAWN), static_cast<Square>(from));
            }
            else {
                move_piece(moving_piece, to, from);
            }

            // 2. Restore captured pieces
            if (flags == MoveFlags::EnPassant) {
                int victim_sq = (us == Color::WHITE) ? to - 8 : to + 8;
                place_piece(Piece(enemy, PieceType::PAWN), static_cast<Square>(victim_sq));
            }
            else if (!prev.captured_piece.is_none()) {
                place_piece(prev.captured_piece, static_cast<Square>(to));
            }

            // 3. Handle Castling
            if (flags == MoveFlags::Castling) {
                auto [r_orig_corner, r_landed] = CASTLE_ROOK[to];
                move_piece(mailbox[r_landed], r_landed, r_orig_corner);
            }

            // 4. Restore Metadata
            metadata = prev.prev_flags;
        }

        void place_piece(Piece piece, Square sq) {
            int idx = bb_index(piece);
            ui64 bit = 1ULL << sq;
            board[idx] |= bit;
            occupancy[static_cast<int>(piece.color()) - 1] |= bit;
            total_pieces |= bit;
            mailbox[sq] = piece;
        }

        void remove_piece(Piece piece, Square sq) {
            int idx = bb_index(piece);
            ui64 bit = 1ULL << sq;
            board[idx] &= ~bit;
            occupancy[static_cast<int>(piece.color()) - 1] &= ~bit;
            total_pieces &= ~bit;
            mailbox[sq] = Piece();
        }

        void move_piece(Piece piece, int from, int to) {
            remove_piece(piece, from);
            place_piece(piece, to);
        }

        void update_occupancy() {
            int white_idx = static_cast<int>(Color::WHITE) - 1;
            int black_idx = static_cast<int>(Color::BLACK) - 1;
            occupancy[white_idx] = 0;
            occupancy[black_idx] = 0;

            for (int i = 0; i < 6; ++i) {
                occupancy[white_idx] |= board[i];
                occupancy[black_idx] |= board[i + 6];
            }

            total_pieces = occupancy[white_idx] | occupancy[black_idx];
        }

        bool is_in_check(Color us, Square king_sq) const {
            return is_square_attacked(king_sq, us);
        }
        bool is_square_attacked(Square sq, Color us) const {
            const Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
            const int them_off = (static_cast<int>(them) - 1) * 6;

            // 1. Leapers & Pawns
            if (AttackTables::get_pawn_attack(sq, us) & board[them_off + 0]) return true; // PieceType::PAWN - 1 = 0
            if (AttackTables::get_knight_attacks(sq) & board[them_off + 2]) return true; // PieceType::KNIGHT - 1 = 2
            if (AttackTables::get_king_attacks(sq) & board[them_off + 5]) return true; // PieceType::KING - 1 = 5

            // 2. Sliders
            // Check Bishops and Queens together
            ui64 bishop_queen = board[them_off + 3] | board[them_off + 4];
            if (AttackTables::get_bishop_attacks(sq, total_pieces) & bishop_queen) return true;

            // Check Rooks and Queens together
            ui64 rook_queen = board[them_off + 1] | board[them_off + 4];
            if (AttackTables::get_rook_attacks(sq, total_pieces) & rook_queen) return true;

            return false;
        }

        ui64 get_attacks_to(Square sq, Color us) const {
            const Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
            ui64 attackers{};

            // Knights
            attackers |= AttackTables::get_knight_attacks(sq) &
                         board[bb_index(Piece(them, PieceType::KNIGHT))];

            // Kings
            attackers |= AttackTables::get_king_attacks(sq) &
                         board[bb_index(Piece(them, PieceType::KING))];

            // Rooks + Queens
            ui64 rook_attacks = AttackTables::get_rook_attacks(sq, total_pieces);
            attackers |= rook_attacks &
                             (board[bb_index(Piece(them, PieceType::ROOK))] |
                              board[bb_index(Piece(them, PieceType::QUEEN))]);

            // Bishops + Queens
            ui64 bishop_attacks = AttackTables::get_bishop_attacks(sq, total_pieces);
            attackers |= bishop_attacks &
                            (board[bb_index(Piece(them, PieceType::BISHOP))] |
                             board[bb_index(Piece(them, PieceType::QUEEN))]);

            // Pawns
            // Pawns attack *towards us*, so use `us` for attack direction
            attackers |= AttackTables::get_pawn_attack(sq, us) &
                         board[bb_index(Piece(them, PieceType::PAWN))];

            return attackers;
        }


        ui64 get_highlight_bitboard(Types::Square sq) {
            Piece piece = mailbox[sq];
            if (piece.is_none()) return 0ULL;

            Color us = piece.color();
            assert(us != Color::NONE);
            int us_idx = static_cast<int>(us) - 1;
            ui64 my_stuff = occupancy[us_idx];
            ui64 enemy_stuff = occupancy[!us_idx];

            ui64 moves{};

            switch (piece.type()) {
            case PieceType::PAWN:   moves = AttackTables::get_pawn_moves_and_attacks(
                                                sq, us, total_pieces, enemy_stuff, metadata.en_passant_file()
                                            ); break;
            case PieceType::KNIGHT: moves = AttackTables::get_knight_attacks(sq); break;
            case PieceType::KING:   moves = AttackTables::get_king_attacks(sq);
                                    moves |= get_castling_targets(us); break;
            case PieceType::ROOK:   moves = AttackTables::get_rook_attacks(sq, total_pieces); break;
            case PieceType::BISHOP: moves = AttackTables::get_bishop_attacks(sq, total_pieces); break;
            case PieceType::QUEEN:  moves = AttackTables::get_queen_attacks(sq, total_pieces); break;
            default: return 0ULL;
            }

            // Mask out friendly fire for everything except Pawns (already handled)
            if (piece.type() != PieceType::PAWN) {
                moves &= ~my_stuff;
            }

            return moves;
        }

        ui64 get_castling_targets(Color us) {
			constexpr ui64 WHITE_KINGSIDE_MASK  = (1ULL << Square::SQ_F1) | (1ULL << Square::SQ_G1);
			constexpr ui64 WHITE_QUEENSIDE_MASK = (1ULL << Square::SQ_D1) | (1ULL << Square::SQ_C1) | (1ULL << Square::SQ_B1);
			constexpr ui64 BLACK_KINGSIDE_MASK  = (1ULL << Square::SQ_F8) | (1ULL << Square::SQ_G8);
			constexpr ui64 BLACK_QUEENSIDE_MASK = (1ULL << Square::SQ_D8) | (1ULL << Square::SQ_C8) | (1ULL << Square::SQ_B8);

            // 1. If currently in check, castling is illegal
            if (is_in_check(us, get_king_square(us))) return 0ULL;

            ui64 targets = 0ULL;
            const int enemy_idx = (us == Color::WHITE) ? 1 : 0;
            const ui64 enemy_occ = occupancy[enemy_idx];

            if (us == Color::WHITE) {
                // Kingside: E1 to G1 (Must check F1)
                if (metadata.can_white_kingside() && !(total_pieces & WHITE_KINGSIDE_MASK)) {
                    // Check if F1 is attacked (G1 check is handled by is_move_legal's final check)
                    if (!is_square_attacked(Square::SQ_F1, us)) targets |= (1ULL << Square::SQ_G1);
                }
                // Queenside: E1 to C1 (Must check D1)
                if (metadata.can_white_queenside() && !(total_pieces & WHITE_QUEENSIDE_MASK)) {
                    // Check if D1 is attacked
                    if (!is_square_attacked(Square::SQ_D1, us)) targets |= (1ULL << Square::SQ_C1);
                }
            }
            else {
                // Black Kingside: E8 to G8 (Must check F8)
                if (metadata.can_black_kingside() && !(total_pieces & BLACK_KINGSIDE_MASK)) {
                    if (!is_square_attacked(Square::SQ_F8, us)) targets |= (1ULL << Square::SQ_G8);
                }
                // Black Queenside: E8 to C8 (Must check D8)
                if (metadata.can_black_queenside() && !(total_pieces & BLACK_QUEENSIDE_MASK)) {
                    if (!is_square_attacked(Square::SQ_D8, us)) targets |= (1ULL << Square::SQ_C8);
                }
            }
            return targets;
        }

        void highlight_attacks(std::ostream& out, Square sq) {
            ui64 attacks = get_highlight_bitboard(sq);
            print_attacks(out, attacks);
        }

        void print_attacks(std::ostream& out, ui64 attacks) const {
            out << "\n    a   b   c   d   e   f   g   h\n";
            out << "  +---+---+---+---+---+---+---+---+\n";
            for (int r = 7; r >= 0; --r) {
                out << r + 1 << " |";
                for (int f = 0; f < 8; ++f) {
                    int sq = r * 8 + f;
                    out << ' ' << ((attacks & (1ULL << sq)) ? '*' : '.') << " |";
                }
                out << " " << r + 1 << '\n';
                out << "  +---+---+---+---+---+---+---+---+\n";
            }
            out << "    a   b   c   d   e   f   g   h\n";
        }
        void print(std::ostream& out) const {
            out << "\n    a   b   c   d   e   f   g   h\n";
            out << "  +---+---+---+---+---+---+---+---+\n";
            for (int r = 7; r >= 0; --r) {
                out << r + 1 << " |";
                for (int f = 0; f < 8; ++f) {
                    int sq = r * 8 + f;
                    Piece piece = mailbox[sq];
                    out << ' ' << piece_to_char(piece) << " |";
                }
                out << " " << r + 1 << '\n';
                out << "  +---+---+---+---+---+---+---+---+\n";
            }
            out << "    a   b   c   d   e   f   g   h\n";
            out << (metadata.side_to_move() == Color::BLACK ? "Black" : "White") << " to move\n";
        }
	};

    // Human move validation helper
    bool is_move_legal(Position& pos, Move m) {
        int from = m.from();
        int to = m.to();

        Piece p = pos.get_mailbox()[from];
        if (p.is_none()) return false;

        Color us = pos.get_metadata().side_to_move();
        if (p.is_white() != (us == Color::WHITE)) return false;

        ui64 targets = pos.get_highlight_bitboard(from);
        if (!(targets & (1ULL << to))) return false;

        pos.make_move(m);
        bool in_check = pos.is_in_check(us, pos.get_king_square(us));
        pos.undo_move();

        return !in_check;
    }
}