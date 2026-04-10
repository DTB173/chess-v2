#pragma once

#include <array>
#include <vector>
#include <ostream>
#include <cassert>
#include <string>
#include <algorithm>
#include <sstream>
#include <memory>

#include "Types.h"
#include "Bitboards.h"
#include "AttackTables.h"
#include "Zobrist.h"
#include "PSQTables.h"
#include "../../../texel-tuner/src/engines/EvalInfo.h"

namespace Position {
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

    constexpr std::array<int, 6> PIECE_PHASE_WEIGHT = {
		0,    // PAWN
        2,    // ROOK
        1,    // KNIGHT
        1,    // BISHOP
        4,    // QUEEN
		0,    // KING
	};
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
		ui8 halfmove_clock{ 0 };

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

        ui16 ep_index() const {
            assert(bits & mask(MASKS::EPValid));
            return (bits & mask(MASKS::EPFile)) >> 4;
        }

        // Castling
        void set_castling_rights(bool wk, bool wq, bool bk, bool bq) {
            bits &= 0xFFF0; // Clear current castling rights
            if (wk) bits |= mask(MASKS::WhiteKingside);
            if (wq) bits |= mask(MASKS::WhiteQueenside);
            if (bk) bits |= mask(MASKS::BlackKingside);
            if (bq) bits |= mask(MASKS::BlackQueenside);
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

        ui16 castling_index()const {
			return bits & 0x0F;
        }

		// Halfmove clock
		void set_clock(ui8 clock) { halfmove_clock = clock; }
		void reset_clock() { halfmove_clock = 0; }
		void increment_clock() { ++halfmove_clock; }
        ui8  get_clock() const { return halfmove_clock; }
        bool is_50_move_draw() const { return halfmove_clock >= 100; }

        // Side to move
        Color side_to_move() const { return bits & mask(MASKS::BlackToMove) ? Color::BLACK : Color::WHITE; }
        void toggle_turn() { bits ^= mask(MASKS::BlackToMove); }
    };

	class Position {
	private:
        struct StateRecord {
            ui64 prev_zobrist;
            ui64 prev_pawn;
            Move prev_half_move;
            Piece captured_piece;
            Flags prev_flags;
        };

        std::unique_ptr<std::array<StateRecord, 2048>> history;
        std::array<ui64, 12> board{};
        std::array<ui64, 2> occupancy{};
        ui64 total_pieces{};

        std::array<Piece, 64> mailbox{};
        ui64 zobrist_key{};
        ui64 pawns_key{};
		Score current_score{};
		int current_phase{};
        Flags metadata{};
        size_t history_idx{};
        Square king_sq[2];
    public:
        Position() : history(std::make_unique<std::array<StateRecord, 2048>>()) {};
        Position(const Position& other)
            : board(other.board),
            occupancy(other.occupancy),
            total_pieces(other.total_pieces),
            mailbox(other.mailbox),
            zobrist_key(other.zobrist_key),
            pawns_key(other.pawns_key),
            current_score(other.current_score),
            current_phase(other.current_phase),
            metadata(other.metadata),
            history_idx(other.history_idx)
        {
            history = std::make_unique<std::array<StateRecord, 2048>>(*other.history);

            king_sq[0] = other.king_sq[0];
            king_sq[1] = other.king_sq[1];
        }

		// prevent accidental copying, allowed to setup for lazy smp
		//Position(const Position& other) = delete;
		//Position& operator=(const Position& other) = delete;

		ui64 get_zobrist_key() const { return zobrist_key; }
        ui64 get_pawn_key() const { return pawns_key; }
        const std::array<Piece, 64>& get_mailbox() const { return mailbox; }
        Flags get_metadata() const { return metadata; }
		inline Color turn() const { return metadata.side_to_move(); }
        inline int get_phase()const { return current_phase; }
        inline Square get_king_square(Color c) const { 
            return king_sq[static_cast<int>(c) - 1];
        }

        inline ui64 get_bitboard(PieceType type, Color c) const {
			return board[bb_index(Piece(c, type))];
        }

		inline ui64 get_occupancy(Color c) const {
            return occupancy[static_cast<size_t>(c) - 1];
		}
        inline ui64 get_total_occupancy()const { return total_pieces; }
        inline int half_move_count()const { return history_idx - 1; }
        inline bool is_legal(Move m) {
            Color us = turn(); // get 'us' before, make_move toogles turn
            make_move(m);
			bool in_check = is_in_check(us);
            undo_move();
			return !in_check;
        }

        inline bool gives_check(Move m) {
            make_move(m);
			bool in_check = is_in_check(turn());
			undo_move();
			return in_check;
        }

        Move get_last_move() const {
            if (history_idx == 0) return NO_MOVE;
            Move m = (*history)[history_idx - 1].prev_half_move;
            return m;
        }
		// zugzwang detection helper
        bool has_non_pawn_material(Color c) const {
            if (c == Color::WHITE) {
                ui64 pieces{};
                for (int i{1}; i < 6; ++i) {
					pieces |= board[i];
                }
                return pieces != 0ULL;
            }
            else {
                ui64 pieces{};
                for (int i{7}; i < 12; ++i) {
                    pieces |= board[i];
                }
                return pieces != 0ULL;
            }
		}

        int evaluate() const {
            // 1. Clamp phase to [0, 24] for safety
            int phase = std::clamp(current_phase, 0, 24);

            // 2. Linear interpolation (Tapering)
            int score = current_score.eval(phase);

            // 3. Return relative to side to move (standard for alpha-beta)
            return score;
        }

        bool is_draw() const {
            // 1. Fifty-move rule
            if (metadata.get_clock() >= 100) {
                return true;
            }

            // Check backwards, but only as far as the 50-move counter allows
            int start = std::max(0, static_cast<int>(history_idx - metadata.get_clock()));

            for (int i = history_idx - 1; i >= start; --i) {
                if ((*history)[i].prev_zobrist == zobrist_key) {
                    return true;
                }
            }

            return false;
        }

        void reset() {
            for (int i = 0; i < 12; ++i) board[i] = 0ULL;
            occupancy[0] = occupancy[1] = 0ULL;
            total_pieces = 0ULL;

            for (int i = 0; i < 64; ++i) mailbox[i] = Piece();

            king_sq[0] = king_sq[1] = Square::SQ_NONE;

            metadata = Flags();

            history_idx = 0;
            std::fill(history->begin(), history->end(), StateRecord{});

            current_score = Score{ 0, 0 };
            current_phase = 0;

            zobrist_key = 0;
            pawns_key = 0;
        }

        bool set_fen(const std::string& fen, EvalInfo::EvalTrace* et = nullptr) {
            using namespace Types;
            reset();

            std::istringstream ss(fen);
            std::string token;

            // 1. Piece placement
            if (!(ss >> token)) return false;

            int rank = 7, file = 0;
            for (char c : token) {
                if (c == '/') {
                    if (file != 8) return false;
                    --rank;
                    file = 0;
                    continue;
                }

                if (std::isdigit(c)) {
                    file += c - '0';
                    continue;
                }

                PieceType type = PieceType::NONE;
                Color color = Color::WHITE; // default, overridden for black

                switch (c) {
                case 'P': type = PieceType::PAWN;   break;
                case 'N': type = PieceType::KNIGHT; break;
                case 'B': type = PieceType::BISHOP; break;
                case 'R': type = PieceType::ROOK;   break;
                case 'Q': type = PieceType::QUEEN;  break;
                case 'K': type = PieceType::KING;   break;
                case 'p': type = PieceType::PAWN;   color = Color::BLACK; break;
                case 'n': type = PieceType::KNIGHT; color = Color::BLACK; break;
                case 'b': type = PieceType::BISHOP; color = Color::BLACK; break;
                case 'r': type = PieceType::ROOK;   color = Color::BLACK; break;
                case 'q': type = PieceType::QUEEN;  color = Color::BLACK; break;
                case 'k': type = PieceType::KING;   color = Color::BLACK; break;
                default: return false;
                }

                if (type == PieceType::NONE || file >= 8) return false;

                int sq = rank * 8 + file;
                Piece piece(color, type);

                int bb_idx = bb_index(piece);
                Bitwise::set_bit(board[bb_idx], sq);
                mailbox[sq] = piece;

                if (type == PieceType::KING) {
                    king_sq[static_cast<int>(color) - 1] = static_cast<Square>(sq);
                }

                ++file;
            }

            if (rank != 0 || file != 8) return false;

            // 2. Side to move
            if (!(ss >> token)) return false;
            if (token == "w") {
                // flags already has BlackToMove = 0
            }
            else if (token == "b") {
                metadata.toggle_turn(); // sets BlackToMove bit
            }
            else {
                return false;
            }

            // 3. Castling rights
            if (!(ss >> token)) return false;
            if (token != "-") {
				bool wk = false, wq = false, bk = false, bq = false;
                for (char c : token) {
                    if (c == 'K') wk = true;
                    else if (c == 'Q') wq = true;
                    else if (c == 'k') bk = true;
                    else if (c == 'q') bq = true;
                }
				metadata.set_castling_rights(wk, wq, bk, bq);
            }
            else {
				metadata.set_castling_rights(false, false, false, false);
            }

            // 4. En passant square
            if (!(ss >> token)) return false;
            if (token != "-") {
                if (token.size() != 2) return false;
                int ep_file = token[0] - 'a';
                int ep_rank = token[1] - '1';
                if (ep_file < 0 || ep_file > 7 || (ep_rank != 2 && ep_rank != 5)) return false;

                metadata.set_en_passant_file(ep_file);
            }

            // 5. Halfmove clock
            if (ss >> token) {
                try {
                    int clock = std::stoi(token);
                    metadata.set_clock(static_cast<ui8>(std::min(clock, 255)));
                }
                catch (...) {
                    return false;
                }

                // 6. Fullmove number (optional to store)
                if (ss >> token) {
                }
            }

            // Rebuild occupancy bitboards
            int white_idx = static_cast<int>(Color::WHITE) - 1;
            int black_idx = static_cast<int>(Color::BLACK) - 1;
            for (int i = 0; i < 6; ++i) {
                occupancy[white_idx] |= board[i];
                occupancy[black_idx] |= board[i + 6];
            }
            total_pieces = occupancy[white_idx] | occupancy[black_idx];

            // Validate kings present
            if (king_sq[white_idx] == Square::SQ_NONE || king_sq[black_idx] == Square::SQ_NONE)
                return false;

            // Recompute zobrist key
            zobrist_key = compute_hash_from_scratch();
            pawns_key = compute_pawn_key();
            // Recompute evaluation phase and score
            current_phase = 0;
            current_score = Score{ 0, 0 };

            for (int sq = 0; sq < 64; ++sq) {
                Piece p = mailbox[sq];
                if (p.type() != PieceType::NONE) {
                    Score psq = PSQTables::get_piece_value(p.type(), p.color(), sq);
                    int sign = (p.color() == Color::WHITE) ? 1 : -1;
                    current_score += psq * sign;

                    if (p.type() > PieceType::PAWN && p.type() < PieceType::KING) {
                        current_phase += PIECE_PHASE_WEIGHT[static_cast<int>(p.type()) - 1];
                    }

                    if (et) {
                        et->add_material((int)p.type(), (p.color() == Color::WHITE) ? 1 : -1);
                        et->add_psq((int)p.type(), sq, (int)p.color());
                    }
                }
            }

            return true;
        }


        // NPM helpers
        void make_null_move() {
            (*history)[history_idx++] = { zobrist_key, pawns_key, Move{}, Piece(), metadata };
            assert(history_idx < history->size());

            if (metadata.en_passant_square() != Square::SQ_NONE) {
                zobrist_key ^= Zobrist::en_passant[metadata.ep_index()];
                metadata.clear_en_passant();
            }

            zobrist_key ^= Zobrist::side;
            metadata.toggle_turn();
        }
        void undo_null_move() {
            StateRecord prev = (*history)[--history_idx];
            metadata = prev.prev_flags;
            zobrist_key = prev.prev_zobrist;
            pawns_key = prev.prev_pawn;
        }

        void make_move(Move m) {
            int from = m.from();
            int to = m.to();
            int flags = m.flags();
            Color us = metadata.side_to_move();
            Piece moving_piece = mailbox[from];
            Piece captured = mailbox[to];

            // 1. History(Must store captured piece for undo)
            (*history)[history_idx++] = { zobrist_key, pawns_key, m, captured, metadata };
            assert(history_idx < history->size());

            // 2. XOR OUT old metadata before we change it
            zobrist_key ^= Zobrist::castling[metadata.castling_index()];
            if (metadata.en_passant_file() != -1)
                zobrist_key ^= Zobrist::en_passant[metadata.ep_index()];

            // 3. Handle Capture
            if (!captured.is_none() && flags != MoveFlags::EnPassant) {
                remove_piece(captured, to);
            }

			// 3b. Update 50-move rule clock
            if (!captured.is_none() || moving_piece.type() == PieceType::PAWN) {
                metadata.reset_clock();
            }
            else {
                metadata.increment_clock();
            }
            // 4. Basic Movement
            move_piece(moving_piece, from, to);

            // 5. Special Cases
			// Pawn Double Push, En Passant, Promotion
            metadata.clear_en_passant();
            metadata.update_castling_rights(from, to);
            if (m.is_promo()) {
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
                move_piece(mailbox[r_from], r_from, r_to); 
            }

            zobrist_key ^= Zobrist::castling[metadata.castling_index()];
            if (metadata.en_passant_file() != -1) zobrist_key ^= Zobrist::en_passant[metadata.ep_index()];
            metadata.toggle_turn();
            zobrist_key ^= Zobrist::side;

            assert(zobrist_key == compute_hash_from_scratch());
            assert(pawns_key == compute_pawn_key());
        }

        void undo_move() {
            StateRecord prev = (*history)[--history_idx];
            const int from = prev.prev_half_move.from();
            const int to = prev.prev_half_move.to();
            const int flags = prev.prev_half_move.flags();
            const Color us = prev.prev_flags.side_to_move();
            const Color enemy = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;

            Piece moving_piece = mailbox[to];

            // 1. Restore the piece that moved
            if (prev.prev_half_move.is_promo()) {
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
			zobrist_key = prev.prev_zobrist;
            pawns_key = prev.prev_pawn;
        }

        void inline place_piece(Piece piece, Square sq) {
            int idx = bb_index(piece);
            ui64 bit = 1ULL << sq;
            board[idx] |= bit;
            occupancy[static_cast<int>(piece.color()) - 1] |= bit;
            total_pieces |= bit;
            mailbox[sq] = piece;
            // XOR IN
            zobrist_key ^= Zobrist::pieces[idx][sq];

            if (piece.type() == PieceType::PAWN) pawns_key^= Zobrist::pieces[idx][sq];

            if (piece.type() == PieceType::KING) king_sq[static_cast<int>(piece.color()) - 1] = sq;

            constexpr int weight[] = { 1, -1 };
			Score psq = PSQTables::get_piece_value(piece.type(), piece.color(), sq);
			current_score += psq * weight[static_cast<int>(piece.color()) - 1];
            if (piece.type() > PieceType::PAWN && piece.type() < PieceType::KING) {
                current_phase += PIECE_PHASE_WEIGHT[static_cast<int>(piece.type()) - 1];
            }
        }

        void inline remove_piece(Piece piece, Square sq) {
            int idx = bb_index(piece);
            ui64 bit = 1ULL << sq;
            board[idx] &= ~bit;
            occupancy[static_cast<int>(piece.color()) - 1] &= ~bit;
            total_pieces &= ~bit;
            mailbox[sq] = Piece();
            // XOR OUT
            zobrist_key ^= Zobrist::pieces[idx][sq];

            if (piece.type() == PieceType::PAWN) pawns_key^= Zobrist::pieces[idx][sq];

			constexpr int weight[] = { 1, -1 };
            Score psq = PSQTables::get_piece_value(piece.type(), piece.color(), sq);
            current_score -= psq * weight[static_cast<int>(piece.color()) - 1];
            if (piece.type() > PieceType::PAWN && piece.type() < PieceType::KING) {
                current_phase -= PIECE_PHASE_WEIGHT[static_cast<int>(piece.type()) - 1];
			}
        }

        void move_piece(Piece piece, int from, int to) {
            remove_piece(piece, from);
            place_piece(piece, to);
        }

        bool is_in_check(Color us) const {
            Square king_sqr = get_king_square(us);
			return get_attacks_to(king_sqr, total_pieces, us) != 0ULL;
        }
        bool is_square_attacked(Square sq, Color us) const {
            int them_off = (us == Color::WHITE) ? 6 : 0;

            // 1. Leapers & Pawns
            if (AttackTables::get_pawn_attacks(sq, us) & board[them_off + 0]) return true; // PieceType::PAWN - 1 = 0
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

        ui64 get_attacks_to(Square sq, ui64 blockers, Color us) const {
            const Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
            ui64 attackers{};

            // Knights
            attackers |= AttackTables::get_knight_attacks(sq) &
                         board[bb_index(Piece(them, PieceType::KNIGHT))];

            // Kings
            attackers |= AttackTables::get_king_attacks(sq) &
                         board[bb_index(Piece(them, PieceType::KING))];

            // Rooks + Queens
            ui64 rook_attacks = AttackTables::get_rook_attacks(sq, blockers);
            attackers |= rook_attacks &
                             (board[bb_index(Piece(them, PieceType::ROOK))] |
                              board[bb_index(Piece(them, PieceType::QUEEN))]);

            // Bishops + Queens
            ui64 bishop_attacks = AttackTables::get_bishop_attacks(sq, blockers);
            attackers |= bishop_attacks &
                            (board[bb_index(Piece(them, PieceType::BISHOP))] |
                             board[bb_index(Piece(them, PieceType::QUEEN))]);

            // Pawns
            // If we were a pawn, do we have attack on enemy pawn?
            attackers |= AttackTables::get_pawn_attacks(sq, us) &
                         board[bb_index(Piece(them, PieceType::PAWN))];

            return attackers;
        }


        ui64 get_highlight_bitboard(Types::Square sq) const {
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

        ui64 get_castling_targets(Color us) const {
			constexpr ui64 WHITE_KINGSIDE_MASK  = (1ULL << Square::SQ_F1) | (1ULL << Square::SQ_G1);
			constexpr ui64 WHITE_QUEENSIDE_MASK = (1ULL << Square::SQ_D1) | (1ULL << Square::SQ_C1) | (1ULL << Square::SQ_B1);
			constexpr ui64 BLACK_KINGSIDE_MASK  = (1ULL << Square::SQ_F8) | (1ULL << Square::SQ_G8);
			constexpr ui64 BLACK_QUEENSIDE_MASK = (1ULL << Square::SQ_D8) | (1ULL << Square::SQ_C8) | (1ULL << Square::SQ_B8);

            // 1. If currently in check, castling is illegal
            if (is_in_check(us)) return 0ULL;

            ui64 targets = 0ULL;

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

        void highlight_attacks(std::ostream& out, Square sq) const {
            ui64 attacks = get_highlight_bitboard(sq);
            print_attacks(out, attacks);
        }

        ui64 compute_hash_from_scratch() const {
            ui64 h{};

            for (int p_idx = 0; p_idx < 12; ++p_idx) {
                ui64 bb = board[p_idx];
                while (bb) {
                    int sq = Bitwise::lsb(bb);
                    h ^= Zobrist::pieces[p_idx][sq];
                    bb &= (bb - 1);
                }
            }
            h ^= Zobrist::castling[metadata.castling_index()];

            if (metadata.en_passant_file() != -1) {
                h ^= Zobrist::en_passant[metadata.ep_index()];
               
            }

            if (metadata.side_to_move() == Color::BLACK) {
                h ^= Zobrist::side;
            }
            return h;
        }

        ui64 compute_pawn_key() const {
            ui64 k{};

            ui64 wpawns = board[0];
            while (wpawns) {
                int sq = Bitwise::pop_lsb(wpawns);
                k ^= Zobrist::pieces[0][sq];
            }

            ui64 bpawns = board[6];
            while (bpawns) {
                int sq = Bitwise::pop_lsb(bpawns);
                k ^= Zobrist::pieces[6][sq];
            }
            return k;
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
            // --- Meta Data ---
            Square ep_sq = metadata.en_passant_square();
            out << "EP: " << (ep_sq != Square::SQ_NONE ?
                std::string{ Types::file_char(ep_sq) } + std::string{ Types::rank_char(ep_sq) }
            : "None") << '\n';

            out << "Castling: "
                << (metadata.can_white_kingside() ? "K" : "")
                << (metadata.can_white_queenside() ? "Q" : "")
                << (metadata.can_black_kingside() ? "k" : "")
                << (metadata.can_black_queenside() ? "q" : "") << "\n";

            out << "Checks: "
                << (is_in_check(Color::WHITE) ? "W+" : "-") << " "
                << (is_in_check(Color::BLACK) ? "B+" : "-") << '\n';

            out << (metadata.side_to_move() == Color::BLACK ? "Black" : "White") << " to move\n";
            
            /*
            // --- Evaluation Debugging ---
            
            out << "----------------------------------\n";
            out << "EVALUATION DEBUG:\n";
            out << "  MG Score: " << current_score.mg << " cp\n";
            out << "  EG Score: " << current_score.eg << " cp\n";
            out << "  Phase   : " << current_phase << " / 24\n";

            // This calls your tapering logic
            int total_eval = evaluate();
            // Flip it back to White's perspective just for the printout
            int white_perspective_eval = (metadata.side_to_move() == Color::WHITE) ? total_eval : -total_eval;
            out << "Final : " << std::showpos << white_perspective_eval << " cp\n";
            out << std::noshowpos;
            out << "----------------------------------\n";
            */
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
        bool in_check = pos.is_in_check(us);
        pos.undo_move();

        return !in_check;
    }
}