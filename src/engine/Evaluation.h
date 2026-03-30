#pragma once

#include <algorithm>

#include "Position.h"
#include "Types.h"
#include "Bitboards.h"
#include "AttackTables.h"
#include "TranspositionTable.h"
#include "PawnTable.h"

namespace Eval {
	using namespace Types;
	using namespace AttackTables;
	using namespace TranspositionTable;

    constexpr int isolated_pawn_penalty_mg = -25; 
    constexpr int isolated_pawn_penalty_eg = -15;
    constexpr int doubled_pawn_penalty_mg = -15;
    constexpr int doubled_pawn_penalty_eg = -30;
    constexpr int connected_bonus_mg = 12;
    constexpr int connected_bonus_eg = 25;
    constexpr int passed_pawn_bonus_mg[8] = { 0, 0,  5, 10, 25,  60, 120, 0 };
    constexpr int passed_pawn_bonus_eg[8] = { 0, 5, 15, 30, 70, 140, 250, 0 };


    constexpr inline int mobility_mg[6][27] = {
        {   0,   0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // pawn
        {  -8,  -4,  0, 3,  7, 10, 14, 16, 19, 20, 22, 23, 24, 25, 26,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // rook 
        { -28, -14, -4, 4, 11, 19, 24, 27, 29,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // knight
        { -23,  -9, -1, 7, 13, 19, 23, 27, 29, 31, 32, 34, 35, 37,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // bishop
        { -18,  -9, -1, 1,  3,  5,  7,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28}, // queen
        {   0,   0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}  // king
    };

    constexpr inline int mobility_eg[6][27] = {
        {   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // pawn
        { -12,  -8, -3,  1,  8, 15, 23, 30, 38, 43, 48, 52, 56, 60, 63,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // rook
        { -32, -18, -8, -1,  9, 19, 24, 27, 29,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // knight 
        { -28, -14, -4,  4, 11, 19, 26, 34, 39, 44, 47, 51, 55, 59,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // bishop
        { -18,  -9, -1,  3,  7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47, 51, 55, 59, 63, 67, 71, 75, 79, 83, 87, 91, 95}, // queen
        {   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}  // king
    };

    constexpr inline int SafetyTable[100] = {
            0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
          18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
          68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
         140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
         260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
         377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
         494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
         500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
         500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
         500, 500, 500, 500, 500, 500, 500, 500, 500, 500
    };

    PawnTable::PawnTable pawns_cache(16);

    bool is_on_board(Square sq) {
        return sq >= Square::SQ_A1 && sq < Square::SQ_NONE;
    }

    ui64 get_extended_king_zone(Square king_sq) {
        ui64 inner = get_king_attacks(king_sq);
        ui64 outer = 0ULL;
        while (inner) {
            Square sq = Bitwise::pop_lsb(inner);
            outer |= AttackTables::get_king_attacks(sq);
        }
        return (get_king_attacks(king_sq) | outer) & ~(1ULL << king_sq);
    }

    int calculate_shield_penalty(const Position::Position& pos, Color side) {
        Square king_sq = pos.get_king_square(side);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, side);
        int file = king_sq % 8;
        int rank = king_sq / 8;

        // Define the direction "forward"
        int forward = (side == Color::WHITE) ? 1 : -1;
        int penalty = 0;

        // Check the three files around the king
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); ++f) {
            ui64 file_mask = Bitboards::file_mask(f);
            // Does a pawn exist on this file in the two ranks in front of the king?
            ui64 shield_zone = file_mask & (Bitboards::rank_mask(rank + forward) | Bitboards::rank_mask(rank + 2 * forward));
            if (!(my_pawns & shield_zone)) {
                penalty += 40; // No pawn protecting this file
            }
        }
        return penalty;
    }

    Score mobility_and_king_safety(const Position::Position& pos, Color side, ui64 passed_pawns_bb) {
        Color opp = opponent_of(side);
        ui64 my_pieces = pos.get_occupancy(side);
        ui64 total = pos.get_total_occupancy();

        // 1. Enemy Pawn Attacks (To identify "unsafe" mobility squares)
        ui64 enemy_pawns_bb = pos.get_bitboard(PieceType::PAWN, opp);
        ui64 enemy_pawn_attacks = (opp == Color::WHITE)
            ? (((enemy_pawns_bb & NOT_A_FILE) << 7) | ((enemy_pawns_bb & NOT_H_FILE) << 9))
            : (((enemy_pawns_bb & NOT_H_FILE) >> 7) | ((enemy_pawns_bb & NOT_A_FILE) >> 9));

        // 2. Safe King Zone (Opponent's King)
        Square opp_king_sq = pos.get_king_square(opp);
        ui64 king_danger_zone = get_extended_king_zone(opp_king_sq);

        // 3. King Shield Penalty (Our King)
        int own_king_shield_penalty = calculate_shield_penalty(pos, side);

        // 4. The Main Loop
        Score score{};
        ui64 sliders_and_knights = my_pieces & ~(pos.get_bitboard(PieceType::PAWN, side) | pos.get_bitboard(PieceType::KING, side));

        int bishop_count = 0;
        int num_attackers = 0;
        int unit_attacks = 0;

        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, side);
        ui64 pawn_attacks = (side == Color::WHITE)
            ? (((my_pawns & NOT_A_FILE) << 7) | ((my_pawns & NOT_H_FILE) << 9))
            : (((my_pawns & NOT_H_FILE) >> 7) | ((my_pawns & NOT_A_FILE) >> 9));
        ui64 pawn_attacks_on_king = pawn_attacks & king_danger_zone;
        if (pawn_attacks_on_king) {
            num_attackers++;  // Count pawns as an attacker if they hit the zone
            unit_attacks += 1 * Bitwise::count(pawn_attacks_on_king);  // Low weight (1) for pawns
        }

        while (sliders_and_knights) {
            int idx = Bitwise::pop_lsb(sliders_and_knights);
            PieceType pt = pos.get_mailbox()[idx].type();

            if (pt == PieceType::BISHOP) bishop_count++;

            if (pt == PieceType::ROOK) {
                int file = idx % 8;
                int rank = idx / 8;
                int relative_rank = (side == Color::WHITE) ? rank : 7 - rank;
                ui64 file_mask = Bitboards::file_mask(file);
                ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, side);
                ui64 opp_pawns = pos.get_bitboard(PieceType::PAWN, opponent_of(side));

                if (relative_rank == 6) { // 7th Rank
                    score.mg += 20;
                    score.eg += 30;
                }

                if (!(file_mask & my_pawns)) {
                    if (!(file_mask & opp_pawns)) {
                        // Full Open File
                        score.mg += 20;
                        score.eg += 15;
                    }
                    else {
                        // Half-Open File
                        score.mg += 10;
                        score.eg += 5;
                    }
                }
            }

            ui64 attacks = 0;
            switch (pt) {
            case PieceType::KNIGHT: attacks = get_knight_attacks(idx); break;
            case PieceType::BISHOP: attacks = get_bishop_attacks(idx, total); break;
            case PieceType::ROOK:   attacks = get_rook_attacks(idx, total); break;
            case PieceType::QUEEN:  attacks = get_queen_attacks(idx, total);  break;
			default: break;
            }

            // Mobility Calculation
            ui64 safe_mobility = attacks & ~(my_pieces | enemy_pawn_attacks);
            int count = std::min((int)Bitwise::count(safe_mobility), 26);
            score.mg += mobility_mg[static_cast<int>(pt) - 1][count];
            score.eg += mobility_eg[static_cast<int>(pt) - 1][count];

            // King Safety Accumulation (Attacking the ENEMY king)
            ui64 attacks_on_king = attacks & king_danger_zone;
            if (attacks_on_king) {
                num_attackers++;
                int weight = (pt == PieceType::QUEEN) ? 8 : (pt == PieceType::ROOK ? 5 : (pt == PieceType::BISHOP ? 3 : 3));
                unit_attacks += weight * Bitwise::count(attacks_on_king);
            }
        }


        ui64 my_rooks = pos.get_bitboard(PieceType::ROOK, side);
        ui64 my_passed = passed_pawns_bb & pos.get_bitboard(PieceType::PAWN, side);

        while (my_passed) {
            int sq = Bitwise::pop_lsb(my_passed);
            ui64 behind_mask = Bitboards::file_mask(sq % 8) & Bitboards::get_rank_mask_behind(sq, side);
            if (my_rooks & behind_mask) {
                score.mg += 20;
                score.eg += 40;
            }
        }

        // 5. King Safety Scoring
        if (num_attackers >= 2) {
            int index = unit_attacks * num_attackers / 1.5;
            int attack_bonus = SafetyTable[std::min(index, 99)];
            if (pos.get_bitboard(PieceType::QUEEN, opp) != 0) {
                score.mg += attack_bonus;
                score.eg += attack_bonus / 4;
            }
            else {
                score.mg += attack_bonus / 2;  // Reduced for queenless
                score.eg += attack_bonus / 2;
            }
        }

        Square own_king_sq = pos.get_king_square(side);
        int king_rank = own_king_sq / 8;  // 0-based rank
        int exposed_penalty = 0;
        if ((side == Color::WHITE && king_rank > 1) || (side == Color::BLACK && king_rank < 6)) {  // Not on back rank
            exposed_penalty += 50;  // Base for non-back-rank king
            // Check open files/diagonals near king
            int king_file = own_king_sq % 8;
            ui64 king_file_mask = Bitboards::file_mask(king_file);
            if (!(king_file_mask & pos.get_bitboard(PieceType::PAWN, side))) {  // Open file in front of king
                exposed_penalty += 30;
            }
            ui64 adj_files = 0;
            if (king_file > 0) adj_files |= Bitboards::file_mask(king_file - 1);
            if (king_file < 7) adj_files |= Bitboards::file_mask(king_file + 1);

            // 2. Apply penalty if those files lack pawns
            if (!(adj_files & pos.get_bitboard(PieceType::PAWN, side))) {
                exposed_penalty += 20;
            }
        }
        score.mg -= exposed_penalty;
        score.eg -= exposed_penalty / 2;

        // 6. Shield and Bonuses
        score.mg -= own_king_shield_penalty;
        score.eg -= own_king_shield_penalty / 4;

        if (bishop_count >= 2) {
            score.mg += 22;
            score.eg += 88;
        }

        return score;
    }

    Score evaluate_pawn_structure(const Position::Position& pos, ui64& passed_pawns) {
        Score net_score{ 0, 0 };
        passed_pawns = 0;

        for (Color side : {Color::WHITE, Color::BLACK}) {
            int multiplier = (side == Color::WHITE) ? 1 : -1;
            ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, side);
            ui64 opp_pawns = pos.get_bitboard(PieceType::PAWN, opponent_of(side));

            // 1. Doubled Pawns (Calculated once per side)
            int doubled_count = 0;
            for (int f = 0; f < 8; ++f) {
                int cnt = Bitwise::count(my_pawns & Bitboards::file_mask(f));
                if (cnt > 1) doubled_count += (cnt - 1);
            }
            net_score.mg += doubled_count * doubled_pawn_penalty_mg * multiplier;
            net_score.eg += doubled_count * doubled_pawn_penalty_eg * multiplier;

            // 2. Connected/Chain Bonus
            // A pawn is "supported" if it is protected by another pawn (diagonal)
            ui64 supported;
            if (side == Color::WHITE) {
                ui64 attacks = ((my_pawns & ~Bitboards::file_mask(0)) << 7) |
                               ((my_pawns & ~Bitboards::file_mask(7)) << 9);
                supported = my_pawns & attacks;
            }
            else {
                ui64 attacks = ((my_pawns & ~Bitboards::file_mask(7)) >> 7) |
                               ((my_pawns & ~Bitboards::file_mask(0)) >> 9);
                supported = my_pawns & attacks;
            }
            int connected_cnt = Bitwise::count(supported);
            net_score.mg += connected_cnt * connected_bonus_mg * multiplier;
            net_score.eg += connected_cnt * connected_bonus_eg * multiplier;

            // 3. Individual Pawn Loop (Isolated & Passed)
            ui64 temp = my_pawns;
            while (temp) {
                int sq = Bitwise::pop_lsb(temp);
                int file = sq % 8;
                int rank = sq / 8;

                // Isolated Penalty
                if (!(my_pawns & Bitboards::adjacent_files_mask(file))) {
                    net_score.mg += isolated_pawn_penalty_mg * multiplier;
                    net_score.eg += isolated_pawn_penalty_eg * multiplier;
                }

                // Passed Pawn Bonus
                if (!(opp_pawns & Bitboards::get_passed_mask(sq, side))) {
                    passed_pawns |= (1ULL << sq);
                    int rel_rank = (side == Color::WHITE) ? rank : 7 - rank;

                    net_score.mg += passed_pawn_bonus_mg[rel_rank] * multiplier;
                    net_score.eg += passed_pawn_bonus_eg[rel_rank] * multiplier;
                }
            }
        }
        return net_score;
    }

	int evaluate(const Position::Position& pos, TT& tt, ui8 current_age) {
		ui64 key = pos.get_zobrist_key();
		auto* entry = tt.probe(key);
		if (entry && entry->static_eval_ != Constants::SCORE_NONE) {
			return entry->static_eval_;
		}

        ui64 pawn_key = pos.get_pawn_key();
        ui64 passed_pawns_bb = 0;
        Score pawn_structure_score;

        auto* pentry = pawns_cache.probe(pawn_key);
        if (pentry) {
            pawn_structure_score = { pentry->score_mg, pentry->score_eg };
            passed_pawns_bb = pentry->passed_pawns;
        }
        else {
            pawn_structure_score = evaluate_pawn_structure(pos, passed_pawns_bb);
            pawns_cache.store(pawn_key, (i16)pawn_structure_score.mg, (i16)pawn_structure_score.eg, passed_pawns_bb);
        }

        Score total_score = pawn_structure_score;
        total_score += mobility_and_king_safety(pos, Color::WHITE, passed_pawns_bb);
        total_score -= mobility_and_king_safety(pos, Color::BLACK, passed_pawns_bb);

        int phase = pos.get_phase();
        int final_eval = total_score.eval(phase) + pos.evaluate();

        tt.store_eval(key, static_cast<i16>(final_eval), current_age);
        return final_eval;
	}
}