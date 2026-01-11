module;
#include <algorithm>
export module Evaluation;
import Position;
import Types;
import Bitboards;
import AttackTables;
import TranspositionTable;

export namespace Eval {
	using namespace Types;
	using namespace AttackTables;
	using namespace TranspositionTable;

    constexpr int mobility_mg[6][27] = {
        {   0,   0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // pawn
        {  -8,  -4,  0, 3,  7, 10, 14, 16, 19, 20, 22, 23, 24, 25, 26,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // rook 
        { -28, -14, -4, 4, 11, 19, 24, 27, 29,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // knight
        { -23,  -9, -1, 7, 13, 19, 23, 27, 29, 31, 32, 34, 35, 37,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // bishop
        { -18,  -9, -1, 1,  3,  5,  7,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28}, // queen
        {   0,   0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}  // king
    };

    constexpr int mobility_eg[6][27] = {
        {   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // pawn
        { -12,  -8, -3,  1,  8, 15, 23, 30, 38, 43, 48, 52, 56, 60, 63,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // rook
        { -32, -18, -8, -1,  9, 19, 24, 27, 29,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // knight 
        { -28, -14, -4,  4, 11, 19, 26, 34, 39, 44, 47, 51, 55, 59,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // bishop
        { -18,  -9, -1,  3,  7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47, 51, 55, 59, 63, 67, 71, 75, 79, 83, 87, 91, 95}, // queen
        {   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}  // king
    };

    bool is_on_board(Square sq) {
        return sq >= Square::SQ_A1 && sq < Square::SQ_NONE;
    }

	int mobility_and_king_safety(const Position::Position& pos, Color side) {
        Color opp = opponent_of(side);
        ui64 my_pieces = pos.get_occupancy(side);
        ui64 total = pos.get_total_occupancy();

        // 1. OPTIMIZATION: O(1) Pawn Attacks
        // Assumes standard bitboard shift logic.
        // Replace with your specific shift functions if different.
        ui64 enemy_pawns_bb = pos.get_bitboard(PieceType::PAWN, opp);
        ui64 enemy_pawn_attacks = 0;
        if (opp == Color::WHITE) {
            enemy_pawn_attacks |= ((enemy_pawns_bb & NOT_A_FILE) << 7); // Capture Left
            enemy_pawn_attacks |= ((enemy_pawns_bb & NOT_H_FILE) << 9); // Capture Right
        }
        else {
            enemy_pawn_attacks |= ((enemy_pawns_bb & NOT_H_FILE) >> 7);
            enemy_pawn_attacks |= ((enemy_pawns_bb & NOT_A_FILE) >> 9);
        }

        // 2. SAFE KING ZONE (Using pre-computed table or strict checks)
        // "Forward bias" implies we care more about squares in front of the king.
        Square opp_king_sq = pos.get_king_square(opp);
        ui64 king_danger_zone = get_king_attacks(opp_king_sq);

        // 3. KING SHIELD (Direction Aware)
        int own_king_shield_penalty = 0;
        Square my_king_sq = pos.get_king_square(side);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, side);

        // Direction calculation
        int up = (side == Color::WHITE) ? 8 : -8;
        int right = (side == Color::WHITE) ? 1 : -1; // Relative "King Right"

        // Simple 3-pawn shield check (rank in front of king)
        // Be careful with board boundaries here!
        Square s1 = my_king_sq + up - 1;
        Square s2 = my_king_sq + up;
        Square s3 = my_king_sq + up + 1;

        // Check bounds before accessing
        if (is_on_board(s1) && !(my_pawns & (1ULL << s1))) own_king_shield_penalty += 30;
        if (is_on_board(s2) && !(my_pawns & (1ULL << s2))) own_king_shield_penalty += 40;
        if (is_on_board(s3) && !(my_pawns & (1ULL << s3))) own_king_shield_penalty += 30;

        // 4. THE LOOP
        Score score{}; // Accumulator
        ui64 sliders_and_knights = my_pieces & ~(pos.get_bitboard(PieceType::PAWN, side) | pos.get_bitboard(PieceType::KING, side));

        int bishop_count = 0;
        int unit_attacks = 0; // Total "weight" of attacks on enemy king

        while (sliders_and_knights) {
            int idx = Bitwise::pop_lsb(sliders_and_knights);
            PieceType pt = pos.get_mailbox()[idx].type();

            if (pt == PieceType::BISHOP) bishop_count++;

            ui64 attacks = 0;
            switch (pt) {
            case PieceType::KNIGHT: attacks = get_knight_attacks(idx); break;
            case PieceType::BISHOP: attacks = get_bishop_attacks(idx, total); break;
            case PieceType::ROOK:   attacks = get_rook_attacks(idx, total); break;
            case PieceType::QUEEN:  attacks = get_queen_attacks(idx, total);  break;
            }

            // Smart Mobility: Don't count squares where we die to a pawn
            attacks &= ~(my_pieces | enemy_pawn_attacks);

            int count = std::min(Bitwise::count(attacks), 26);
            score.mg += mobility_mg[static_cast<int>(pt) - 1][count];
            score.eg += mobility_eg[static_cast<int>(pt) - 1][count];

            // King Safety Accumulation
            ui64 attacks_on_king = attacks & king_danger_zone;
            if (attacks_on_king) {
                int weight = (pt == PieceType::QUEEN) ? 5 : (pt == PieceType::ROOK ? 3 : 2);
                unit_attacks += weight * Bitwise::count(attacks_on_king);
            }
        }

        // 5. SCORING 
        // Bonus for attacking him
        int king_attack_bonus = 0;
        if (unit_attacks >= 2) {
            king_attack_bonus = unit_attacks * unit_attacks * 2;
            if (unit_attacks >= 8) king_attack_bonus = 80 + (unit_attacks * 5);
        }

        score.mg += king_attack_bonus;
        score.eg += king_attack_bonus / 2; // Attacks matter less in endgame

        score.mg -= own_king_shield_penalty;
        // Shield matters less in endgame, but still non-zero
        score.eg -= own_king_shield_penalty / 4;

        if (bishop_count >= 2) {
            score.mg += 40;
            score.eg += 60;
        }

        int phase = pos.get_phase();
        return score.eval(phase);
    }

	int evaluate(const Position::Position& pos, TT& tt, ui8 current_age) {
		ui64 key = pos.get_zobrist_key();
		auto* entry = tt.probe(key);
		if (entry && entry->static_eval != Constants::SCORE_NONE) {
			return entry->static_eval;
		}
		int score = pos.evaluate();
		score += mobility_and_king_safety(pos, Color::WHITE);
		score -= mobility_and_king_safety(pos, Color::BLACK);

		tt.store_eval(key, static_cast<i16>(score), current_age);
		return score;
	}
}