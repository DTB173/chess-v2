#pragma once

#include <algorithm>

#include "../../../texel-tuner/src/engines/EvalInfo.h"

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

    PawnTable::PawnTable pawns_cache(16);

    struct EvalCache {
        ui64 pawn_attack[2];
        ui64 pawn_attacks_by_2[2];
        ui64 rammed_pawns[2];
        ui64 blocked_pawns[2];
        ui64 king_areas[2];
        ui64 mobility_areas[2];
        ui64 attacked[2];
        ui64 attacked_by_2[2];
        ui64 attacked_by[2][6];

        ui64 occupied_minus_bishops[2];
        ui64 occupied_minus_rooks[2];
        ui64 passed_pawns;
        Square king_sq[2];
        int king_attacks_count[2];
        int king_attackers_count[2];
        Score king_attackers_weight[2];

        Score pk_eval[2];
        Score pk_safety[2];
        PawnTable::PawnEntry* pkentry = nullptr;

        void init(const Position::Position& pos) {
            ui64 white = pos.get_occupancy(Color::WHITE);
            ui64 black = pos.get_occupancy(Color::BLACK);

            ui64 wpawns = pos.get_bitboard(PieceType::PAWN, Color::WHITE);
            ui64 bpawns = pos.get_bitboard(PieceType::PAWN, Color::BLACK);

            ui64 wbishops = pos.get_bitboard(PieceType::BISHOP, Color::WHITE) | pos.get_bitboard(PieceType::QUEEN, Color::WHITE);
            ui64 bbishops = pos.get_bitboard(PieceType::BISHOP, Color::BLACK) | pos.get_bitboard(PieceType::QUEEN, Color::BLACK);

            ui64 wrooks = pos.get_bitboard(PieceType::ROOK, Color::WHITE) | pos.get_bitboard(PieceType::QUEEN, Color::WHITE);
            ui64 brooks = pos.get_bitboard(PieceType::ROOK, Color::BLACK) | pos.get_bitboard(PieceType::QUEEN, Color::BLACK);

            ui64 wking = pos.get_bitboard(PieceType::KING, Color::WHITE);
            ui64 bking = pos.get_bitboard(PieceType::KING, Color::BLACK);

            pawn_attack[0] = pawn_attack_span<Color::WHITE>(wpawns, ~0ULL);
            pawn_attack[1] = pawn_attack_span<Color::BLACK>(bpawns, ~0ULL);

            pawn_attacks_by_2[0] = pawn_attack_double<Color::WHITE>(wpawns, ~0ull);
            pawn_attacks_by_2[1] = pawn_attack_double<Color::BLACK>(bpawns, ~0ull);

            rammed_pawns[0] = pawn_advance<Color::BLACK>(bpawns, ~(wpawns));
            rammed_pawns[1] = pawn_advance<Color::WHITE>(wpawns, ~(bpawns));

            blocked_pawns[0] = pawn_advance<Color::BLACK>(white | black, ~(wpawns));
            blocked_pawns[1] = pawn_advance<Color::WHITE>(white | black, ~(bpawns));

            king_sq[0] = Bitwise::lsb(wking);
            king_sq[1] = Bitwise::lsb(bking);
            king_areas[0] = Bitboards::king_area(king_sq[0]);
            king_areas[1] = Bitboards::king_area(king_sq[1]);

            mobility_areas[0] = ~(pawn_attack[1] | wking | blocked_pawns[0]);
            mobility_areas[1] = ~(pawn_attack[0] | bking | blocked_pawns[1]);

            attacked[0] = attacked_by[0][5] = AttackTables::get_king_attacks(king_sq[0]);
            attacked[1] = attacked_by[1][5] = AttackTables::get_king_attacks(king_sq[1]);

            occupied_minus_bishops[0] = (white | black) ^ (wbishops);
            occupied_minus_bishops[1] = (white | black) ^ (bbishops);

            occupied_minus_rooks[0] = (white | black) ^ (wrooks);
            occupied_minus_rooks[1] = (white | black) ^ (brooks);

            king_attacks_count[0] = king_attacks_count[1] = {};
            king_attackers_count[0] = king_attackers_count[1] = {};
            king_attackers_weight[0] = king_attackers_weight[1] = {};

            pkentry = pawns_cache.probe(pos.get_pawn_key());

            passed_pawns = pkentry ? pkentry->passed_pawns : 0ULL;
            pk_eval[0] = pkentry ? Score{ pkentry->score_mg, pkentry->score_eg } : Score{};
            pk_eval[1] = {};
            pk_safety[0] = pkentry ? Score{ pkentry->safety_w.mg, pkentry->safety_w.eg } : Score{};
            pk_safety[1] = pkentry ? Score{ pkentry->safety_b.mg, pkentry->safety_b.eg } : Score{};
        }
    };


    constexpr inline Score pawn_passed_candidate[2][8] = {
        { {   0,    0}, { -10,  -20}, { -15,   20}, { -15,   30}, { -20,   60}, {  20,   60}, {   0,    0}, {   0,    0} },
        { {   0,    0}, { -10,   20}, {  -5,   25}, {   0,   50}, {  20,  110}, {  50,   70}, {   0,    0}, {   0,    0} }
    };

    constexpr inline Score pawn_isolated[8] = {
        { -10,  -10}, {   0,  -15}, {   0,  -15}, {   0,  -20},
        {   0,  -20}, {   0,  -15}, {  -5,  -15}, {  -5,  -15}
    };

    constexpr inline Score pawn_stacked[2][8] = {
        { {  10,  -30}, {   0,  -25}, {   0,  -20}, {   0,  -20}, {   5,  -20}, {   5,  -25}, {   5,  -30}, {   5,  -30} },
        { {   3,  -15}, {   0,  -15}, {  -5,  -10}, {  -5,  -10}, {  -5,  -10}, {   0,  -10}, {   0,  -10}, {   0,  -15} }
    };

    constexpr inline Score pawn_backward[2][8] = {
        { {   0,    0}, {   0,   -5}, {   5,   -5}, {   5,  -15}, {  -5,  -30}, {   0,    0}, {   0,    0}, {   0,    0} },
        { {   0,    0}, { -10,  -30}, {  -5,  -30}, {   5,  -30}, {  30,  -40}, {   0,    0}, {   0,    0}, {   0,    0} }
    };

    constexpr inline Score pawn_connected32[32] = {
        {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0},
        {   0,  -10}, {  10,   -5}, {   0,    0}, {   5,   10},
        {  15,    0}, {  20,   -5}, {  20,    0}, {  15,   10},
        {   5,    0}, {  20,    0}, {   5,    0}, {  15,   10},
        {  10,   15}, {  20,   20}, {  30,   20}, {  25,   20},
        {  45,   40}, {  35,   65}, {  60,   75}, {  65,   90},
        { 110,   35}, { 215,   45}, { 215,   70}, { 235,   60},
        {   0,    0}, {   0,    0}, {   0,    0}, {   0,    0}
    };

    constexpr inline Score knight_outpost[2][2] = {
        { {  10,   -5}, {  40,    0} },
        { {   5,   -5}, {  20,   -5} }
    };

    constexpr inline Score knight_behind_pawn = { 0,   30 };

    constexpr inline Score knight_in_narnia[4] = {
        { -10,   -5}, { -10,  -20}, { -30,  -20}, { -50,  -20}
    };

    constexpr inline Score bishop_pair = { 20,   90 };

    constexpr inline Score bishop_rammed_pawns = { -10, -15 };
    constexpr inline Score bishop_outpost[2][2] = {
        { {  15,  -15}, {  50,   -5} },
        { {  10,  -10}, {   5,    5} }
    };

    constexpr inline Score bishop_behind_pawn = { 5,   25 };

    constexpr inline Score bishop_long_diagonal = { 25,   20 };

    constexpr inline Score rook_file[2] = {
        {  10,   10}, {  35,   10}
    };

    constexpr inline Score rook_on_seventh = { 0,   40 };

    constexpr inline Score queen_relative_pin = { -20,  -15 };

    constexpr inline Score king_pawn_proximity[8] = {
        { 35,  45 },{ 20,  30 },{ 15,  15 },{ -10, -20 },
        { -5, -60 },{ -5, -75 },{ -15, -80 },{ -10, -75 },
    };

    constexpr inline Score king_shelter[2][8][8] = {
    {
        { { -5, -5 },{ 15, -30 },{ 25, -5 },{ 25, 10 },{ 5, 0 },{ -10, 5 },{ -15, -35 },{ -60, 25 } },
        { { 10, -5 },{ 0, -15 },{ -5, 0 },{ 5, -5 },{ -10, 5 },{ -55, 70 },{ 80, 80 },{ -20, 0 } },
        { { 40, -5 },{ 5, -5 },{ -35, 5 },{ -15, -15 },{ -10, -5 },{ -25, 10 },{ 10, 75 },{ -15, 0 } },
        { { 20, 10 },{ 25, -20 },{ 0, -15 },{ 10, -20 },{ 20, -35 },{ -50, 10 },{ -140, 50 },{ -5, -5 } },
        { { -10, 15 },{ 0, -5 },{ -45, 5 },{ -30, 10 },{ -25, 0 },{ -35, -5 },{ 40, -25 },{ -15, 5 } },
        { { 50, -15 },{ 15, -15 },{ -25, 5 },{ -10, -20 },{ 10, -35 },{ 35, -20 },{ 50, -40 },{ -20, 0 } },
        { { 40, -15 },{ 0, -25 },{ -30, 0 },{ -25, -10 },{ -30, 0 },{ -20, 30 },{ 5, 50 },{ -15, 5 } },
        { { 10, -20 },{ 0, -25 },{ 10, 0 },{ 0, 15 },{ -10, 25 },{ -10, 45 },{ -185, 80 },{ -15, 15 } }
    },
    {
        { { 0, 0 },{ -15, -40 },{ 10, -30 },{ -50, 15 },{ -35, 5 },{ -10, 50 },{ -170, 0 },{ -60, 20 } },
        { { 0, 0 },{ 15, -20 },{ 10, -10 },{ -10, -5 },{ 0, -25 },{ 25, 75 },{ -185, 0 },{ -30, 10 } },
        { { 0, 0 },{ 20, -10 },{ 0, -10 },{ 10, -25 },{ 30, -5 },{ -90, 55 },{ -90, -75 },{ -10, 0 } },
        { { 0, 0 },{ 0, 5 },{ -5, -5 },{ -35, 10 },{ -45, 15 },{ -100, 35 },{ -10, -45 },{ -35, -5 } },
        { { 0, 0 },{ 10, -5 },{ 15, -15 },{ 15, -15 },{ 0, -15 },{ -35, 5 },{ -100, -50 },{ -20, 0 } },
        { { 0, 0 },{ -10, -5 },{ -20, 0 },{ -15, -5 },{ 25, -20 },{ -25, 10 },{ 50, 40 },{ -15, 0 } },
        { { 0, 0 },{ 30, -20 },{ 20, -15 },{ -10, -5 },{ -30, 15 },{ -5, 25 },{ -50, -40 },{ -30, 20 } },
        { { 0, 0 },{ 15, -60 },{ 15, -30 },{ -20, -5 },{ -30, 25 },{ -10, 25 },{ -225, -50 },{ -30, 0 } }
    }
    };

    constexpr inline Score king_storm[2][4][8] = {
        {
            { { -5, 35 },{ 145, -5 },{ -15, 25 },{ -5, 0 },{ -10, 0 },{ -10, -5 },{ -20, 10 },{ -30, 0 } },
            { { -20, 60 },{ 65, 15 },{ -10, 20 },{ 10, 10 },{ 0, 10 },{ 5, 0 },{ -5, 0 },{ -15, 10 } },
            { { 0, 50 },{ 15, 30 },{ -15, 20 },{ -15, 10 },{ 0, 5 },{ 5, 5 },{ 10, -5 },{ 5, 10 } },
            { { 0, 25 },{ 15, 20 },{ -30, 10 },{ -20, 0 },{ -15, 5 },{ 15, -10 },{ 0, -5 },{ -20, 10 } }
        },
        {
            { { 0, 0 },{ -20, -15 },{ -20, 0 },{ 25, -25 },{ 10, -5 },{ 15, -25 },{ -5, 10 },{ 10, 30 } },
            { { 0, 0 },{ -15, -40 },{ 0, -15 },{ 55, -15 },{ 15, -5 },{ 20, -30 },{ -10, -15 },{ -30, 5 } },
            { { 0, 0 },{ -35, -60 },{ -15, -15 },{ 10, -5 },{ 5, 0 },{ 0, -15 },{ 0, -20 },{ 0, 0 } },
            { { 0, 0 },{ 0, -25 },{ -25, -20 },{ -20, 5 },{ -10, -5 },{ 5, -35 },{ 65, -30 },{ 10, 25 } }
        }
    };

    constexpr inline Score king_defenders[12] = {
        { -35, 0 },{ -15,  0 },{ 0,    5 },{ 10, 10 },
        { 20, 10 },{  30,  0 },{ 40, -15 },{ 10, -5 },
        { 10,  5 },{  10,  5 },{ 10,   5 },{ 10,  5 },
    };

    constexpr inline Score passed_pawn[2][2][8] = {
        {
            { {   0,    0}, { -40,   -5}, { -40,   25}, { -60,   30}, {  10,   20}, { 100,   -5}, { 160,   45}, {   0,    0} },
            { {   0,    0}, { -30,   15}, { -40,   40}, { -55,   45}, {   0,   55}, { 115,   55}, { 195,   95}, {   0,    0} }
        },
        {
            { {   0,    0}, { -30,   30}, { -45,   35}, { -60,   55}, {  10,   65}, { 105,   75}, { 260,  125}, {   0,    0} },
            { {   0,    0}, { -30,   25}, { -40,   35}, { -55,   60}, {  10,   90}, {  95,  165}, { 125,  295}, {   0,    0} }
        }
    };

    constexpr inline Score safety_shelter[2][8] = {
        { 
            { 0, 5 },{ 0, 15 },{ 0, 10 },{ 5,   5 },
            { 5, 0 },{ 0,  0 },{ 0,  0 },{ 0, -15 } 
        },
        {
            { 0, 0 },{  0, 15 },{ 0, 10 },{ 5,   5 },
            { 5, 0 },{ -5,  0 },{ 0,  0 },{ 0, -10 } 
        }
    };

    constexpr inline Score safety_storm[2][8] = {
        { 
            { -5, 0 },{ -10,  5 },{ 0,  5 },{ 0,   0 },
            {  5, 5 },{   0, 20 },{ 0, 20 },{ 0, -10 } 
        },
        { 
            { 0, 0 },{ 0, 0 },{ 0, 5 },{ 0, 0 },
            { 0, 5 },{ 0, 0 },{ 0, 0 },{ 0, 0 } 
        }
    };

    constexpr inline Score passed_friendly_distance[8] = {
        {   0,    0}, {  -3,    1}, {   0,   -4}, {   5,  -13},
        {   6,  -19}, {  -9,  -19}, {  -9,   -7}, {   0,    0}
    };

    constexpr inline Score passed_enemy_distance[8] = {
        {   0,    0}, {   5,   -1}, {   7,    0}, {   9,   11},
        {   0,   25}, {   1,   37}, {  16,   37}, {   0,    0}
    };

    constexpr inline Score passed_safe_promo_path = { -49,   57 };

    constexpr inline Score knight_mobility[10] = {
        {-30, -40}, {-20, -25}, {-10, -10}, { -5,   0},
        {  0,   5}, {  5,  10}, { 10,  20}, { 20,  25},
        { 30,  20}, { 35,  10}
    };

    constexpr inline Score bishop_mobility[14] = {
        {-40, -50}, {-25, -30}, {-10, -15}, { -5,  -5},
        {  0,   0}, {  5,  10}, { 10,  20}, { 15,  30},
        { 20,  35}, { 25,  40}, { 30,  45}, { 35,  40},
        { 40,  35}, { 45,  30}
    };

    constexpr inline Score rook_mobility[15] = {
        {-30, -40}, {-20, -25}, {-10, -10}, { -5,   0},
        {  0,   5}, {  5,  15}, { 10,  25}, { 15,  35},
        { 20,  45}, { 25,  50}, { 30,  55}, { 35,  60},
        { 40,  60}, { 45,  55}, { 50,  50}
    };

    constexpr inline Score queen_mobility[28] = {
        {-20, -30}, {-15, -25}, {-10, -20}, { -5, -15},
        { -5, -10}, {  0,  -5}, {  0,   0}, {  5,   5},
        {  5,  10}, { 10,  15}, { 10,  20}, { 15,  25},
        { 15,  30}, { 20,  35}, { 20,  40}, { 25,  45},
        { 25,  45}, { 25,  45}, { 25,  40}, { 20,  35},
        { 20,  30}, { 15,  25}, { 10,  20}, {  5,  10},
        {  0,   5}, { -5,   0}, {-10,  -5}, {-15, -10}
    };

    constexpr inline Score safety_knight_weight = { 50, 40 };
    constexpr inline Score safety_bishop_weight = { 25, 35 };
    constexpr inline Score safety_rook_weight   = { 35, 10 };
    constexpr inline Score safety_queen_weight  = { 30,  5 };

    constexpr inline Score safety_attack_value = { 45,  35 };
    constexpr inline Score safety_weak_squares = { 40,  40 };
    constexpr inline Score safety_no_enemy_queens = { -235, -260 };
    constexpr inline Score safety_safe_queen_check = { 95,  85 };
    constexpr inline Score safety_safe_rook_check = { 90, 100 };
    constexpr inline Score safety_safe_bishop_check = { 60,  60 };
    constexpr inline Score safety_safe_knight_check = { 110, 115 };
    constexpr inline Score safety_adjustment = { -75, -25 };

    constexpr inline Score safety_table[100] = {
        {   0,    0}, {   0,    0}, {   1,    0}, {   2,    0},
        {   3,    0}, {   5,    0}, {   7,    0}, {   9,    0},
        {  12,    0}, {  15,    0}, {  18,    0}, {  22,    0},
        {  26,    0}, {  30,    0}, {  35,    0}, {  39,    0},
        {  44,    0}, {  50,    0}, {  56,    0}, {  62,    0},
        {  68,    0}, {  75,    0}, {  82,    0}, {  85,    0},
        {  89,    0}, {  97,    0}, { 105,    0}, { 113,    0},
        { 122,    0}, { 131,    0}, { 140,    0}, { 150,    0},
        { 169,    0}, { 180,    0}, { 191,    0}, { 202,    0},
        { 213,    0}, { 225,    0}, { 237,    0}, { 248,    0},
        { 260,    0}, { 272,    0}, { 283,    0}, { 295,    0},
        { 307,    0}, { 319,    0}, { 330,    0}, { 342,    0},
        { 354,    0}, { 366,    0}, { 377,    0}, { 389,    0},
        { 401,    0}, { 412,    0}, { 424,    0}, { 436,    0},
        { 448,    0}, { 459,    0}, { 471,    0}, { 483,    0},
        { 494,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0},
        { 500,    0}, { 500,    0}, { 500,    0}, { 500,    0}
    };

    constexpr inline Score tempo = { 15, 15 };

    

    using M = EvalInfo::Mapper;
    using Pos = Position::Position;

    ui64 discovered_attacks(const Pos& pos, Square sq, Color us) {
        Color them = opponent_of(us);

        ui64 occupied = pos.get_total_occupancy();

        ui64 rook_attacks   = get_rook_attacks(sq, occupied);
        ui64 bishop_attacks = get_bishop_attacks(sq, occupied);

        ui64 enemy_rooks   = pos.get_bitboard(PieceType::ROOK, them) & ~rook_attacks;
        ui64 enemy_bishops = pos.get_bitboard(PieceType::BISHOP, them) & ~bishop_attacks;

        return (enemy_rooks & get_rook_attacks(sq, occupied & ~rook_attacks))
            | (enemy_bishops & get_bishop_attacks(sq, occupied & ~bishop_attacks));
    }

    template<bool Trace, Color us>
    Score evaluate_pawns(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);
        constexpr int forward = (us == Color::WHITE) ? 8 : -8;
        constexpr int us_idx = static_cast<int>(us) - 1;
        constexpr int them_idx = static_cast<int>(them) - 1;

        ec.attacked_by_2[us_idx] = ec.pawn_attack[us_idx] & ec.attacked[us_idx];
        ec.attacked[us_idx]     |= ec.pawn_attack[us_idx];
        ec.attacked_by[us_idx][0] = ec.pawn_attack[us_idx];

        ui64 attacks = ec.pawn_attack[us_idx] & ec.king_areas[them_idx];
        ec.king_attacks_count[them_idx] += Bitwise::count(attacks);

        Score score{}, pkeval{};
        if (ec.pkentry) return score;
        
        ui64 my_pawns    = pos.get_bitboard(PieceType::PAWN, us);
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);

        int sign = us == Color::WHITE ? 1 : -1;
       
        ui64 temp = my_pawns;
        while (temp) {
            Square sq = Bitwise::pop_lsb(temp);
            int relative_rank = Bitboards::relative_rank_of(sq, us);
            int file  = Bitboards::file_of(sq);

            ui64 neighbours   = my_pawns    & Bitboards::adjacent_files_mask(file);
            ui64 backup       = my_pawns    & Bitboards::get_passed_mask(sq, them);
            ui64 stoppers     = enemy_pawns & Bitboards::get_passed_mask(sq, us);
            ui64 threats      = enemy_pawns & get_pawn_attacks(sq, us);
            ui64 support      = my_pawns    & get_pawn_attacks(sq, them);
            ui64 push_threats = enemy_pawns & get_pawn_attacks(sq + forward, us);
            ui64 push_support = my_pawns    & get_pawn_attacks(sq + forward, them);
            ui64 leftovers    = stoppers ^ threats ^ push_threats;

            // Passed Pawn
            if (!stoppers) {
                Bitwise::set_bit(ec.passed_pawns, sq);
            }
            // Candidate Passed Pawn
            else if (!leftovers && Bitwise::count(push_support) >= Bitwise::count(push_threats)) {
                bool supported = (Bitwise::count(support) >= Bitwise::count(threats));
                pkeval += pawn_passed_candidate[supported][relative_rank];

                if constexpr (Trace) {
                    et->add_2d(M::PassedPawnCandidate, supported, relative_rank, 8 , sign);
                }
            }

            // Isolated Pawn
            if (neighbours == 0 && threats == 0) {
                pkeval += pawn_isolated[file];
                if constexpr (Trace) {
                    et->add_1d(M::PawnIsolated, file, sign);
                }
            }

            // Stacked / Doubled Pawns
            if (Bitwise::count(Bitboards::file_mask(file) & my_pawns) > 1) {
                bool can_undouble = (stoppers && (threats || neighbours))
                    || (stoppers & ~Bitboards::forward_file_mask(sq, us));
                pkeval += pawn_stacked[can_undouble][file];
                if constexpr (Trace) {
                    et->add_2d(M::PawnStacked, can_undouble, file, 8, sign);
                }
            }

            // Backward Pawn
            if (neighbours && push_threats && !backup) {
                bool backwards_flag = !(Bitboards::file_mask(file) & enemy_pawns);
                pkeval += pawn_backward[backwards_flag][relative_rank];
                if constexpr (Trace) {
                    et->add_2d(M::PawnBackwards, backwards_flag, relative_rank, 8, sign);
                }
            }
            // Connected Pawns
            else if ((AttackTables::get_connected_mask(sq, us) & my_pawns)) {
                int relative_sq32 = Bitboards::relative_square32(sq, us);
                pkeval += pawn_connected32[relative_sq32];
                if constexpr (Trace) {
                    et->add_1d(M::PawnConnected, relative_sq32, sign);
                }
            }
        }

        ec.pk_eval[us_idx] = pkeval;
        return score;
    }

    template<bool Trace, Color us>
    Score evaluate_knights(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 pawns = enemy_pawns | my_pawns;
        ui64 temp = pos.get_bitboard(PieceType::KNIGHT, us);

        Square my_ksq = pos.get_king_square(us);
        Square enemy_ksq = pos.get_king_square(them);

        constexpr int sign = us == Color::WHITE ? 1 : -1;
        constexpr int us_idx = static_cast<int>(us) - 1;

        ec.attacked_by[us_idx][2] = 0ULL;
        Score score{};

        while (temp) {
            Square sq = Bitwise::pop_lsb(temp);

            ui64 attacks = AttackTables::get_knight_attacks(sq);
            ec.attacked_by_2[us_idx] |= attacks & ec.attacked[us_idx];
            ec.attacked[us_idx] |= attacks;
            ec.attacked_by[us_idx][2] |= attacks;

            // Outpost: Only if it can't be chased by enemy pawns
            if (Bitwise::test_bit(Bitboards::get_outpost_rank_mask(us), sq)
                && !(Bitboards::outpost_square_mask(sq, us) & enemy_pawns))
            {
                bool outside = Bitwise::test_bit(Bitboards::file_mask(1) | Bitboards::file_mask(7), sq);
                bool defended = Bitwise::test_bit(ec.pawn_attack[us_idx], sq);
                score += knight_outpost[outside][defended];

                if constexpr (Trace) {
                    et->add_2d(M::KnightOutpost, outside, defended, 2, sign);
                }
            }

            // Knight behind pawn 
            
            if (Bitwise::test_bit(AttackTables::pawn_advance<them>(pawns, 0ULL), sq)) {
                score += knight_behind_pawn;

                if constexpr (Trace) {
                    et->add((size_t)M::KnightBehindPawn, sign);
                }
            }

            // Proximity: Chebyshev distance
            int dist = std::min(Bitboards::distance(sq, my_ksq), Bitboards::distance(sq, enemy_ksq));
            if (dist >= 4) {
                int d = std::min(dist - 4, 3);
                score += knight_in_narnia[d];

                if constexpr (Trace) {
                    et->add_1d(M::KnightInNarnia, d, sign);
                }
            }

            // 4. Mobility
            ui64 count = Bitwise::count(attacks & ec.mobility_areas[us_idx]);
            score += knight_mobility[count];

            if constexpr (Trace) {
                et->add_1d(M::KnightMobility, count, sign);
            }

            constexpr int enemy_idx = us_idx == 0 ? 1 : 0;
            if ((attacks &= ec.king_areas[enemy_idx] & ~ec.pawn_attacks_by_2[enemy_idx])) {
                ec.king_attacks_count[enemy_idx] += Bitwise::count(attacks);
                ec.king_attackers_count[enemy_idx] += 1;
                ec.king_attackers_weight[enemy_idx] += safety_knight_weight;
            }

            // TODO: Add trace
        }
        return score;
    }

    template<bool Trace, Color us>
    Score evaluate_bishops(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);
        constexpr int us_idx = static_cast<int>(us) - 1;

        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 my_bishops = pos.get_bitboard(PieceType::BISHOP, us);

        ec.attacked_by[us_idx][3] = 0ull;
        int sign = us == Color::WHITE ? 1 : -1;

        // Bishop pair bonus
        Score score{};
        if ((my_bishops & Bitboards::WHITE_SQUARES) && (my_bishops & Bitboards::BLACK_SQUARES)) {
            score += bishop_pair;

            if constexpr (Trace) {
                et->add((size_t)M::BishopPair, sign);
            }
        }

        while (my_bishops) {
            Square sq = Bitwise::pop_lsb(my_bishops);

            ui64 attacks = AttackTables::get_bishop_attacks(sq, ec.occupied_minus_bishops[us_idx]);
            ec.attacked_by_2[us_idx]  |= attacks & ec.attacked[us_idx];
            ec.attacked[us_idx]       |= attacks;
            ec.attacked_by[us_idx][3] |= attacks;

            int count = Bitwise::count(ec.rammed_pawns[us_idx] & Bitboards::squares_matching_color(sq));
            score += count * bishop_rammed_pawns;

            if constexpr (Trace) {
                // TODO: Trace
            }
            
            // Outpost
            if (Bitwise::test_bit(Bitboards::get_outpost_rank_mask(us), sq)
                && !(Bitboards::outpost_square_mask(sq, us) & enemy_pawns))
            {
                bool outside = (Bitboards::file_of(sq) == 0 || Bitboards::file_of(sq) == 7);
                bool defended = Bitwise::test_bit(ec.pawn_attack[us_idx], sq);
                score += bishop_outpost[outside][defended];

                if constexpr (Trace) {
                    et->add_2d(M::BishopOutpost, outside, defended, 2, sign);
                }
            }

            // Bishop behind pawn
            if (Bitwise::test_bit(AttackTables::pawn_advance<them>(my_pawns | enemy_pawns, 0ULL), sq)) {
                score += bishop_behind_pawn;
                if constexpr (Trace) {
                    et->add((size_t)M::BishopBehindPawn, sign);
                }
            }

            // Center control
            if (Bitwise::test_bit(Bitboards::LONG_DIAGONALS & ~Bitboards::CENTER_SQUARES, sq)
                && Bitboards::several(get_bishop_attacks(sq, my_pawns | enemy_pawns) & Bitboards::CENTER_SQUARES))
            {
                score += bishop_long_diagonal;

                if constexpr (Trace) {
                    et->add((size_t)M::BishopLongDiagional, sign);
                }
            }

            // Mobility
            int mobility = Bitwise::count(ec.mobility_areas[us_idx] & attacks);
            score += bishop_mobility[mobility];

            if constexpr (Trace) {
                et->add_1d(M::BishopMobility, mobility, sign);
            }

            constexpr int them_idx = us_idx == 0 ? 1 : 0;
            if (attacks &= ec.king_areas[them_idx] & ~ec.pawn_attacks_by_2[them_idx]) {
                ec.king_attacks_count[them_idx] += Bitwise::count(attacks);
                ec.king_attackers_count[them_idx] += 1;
                ec.king_attackers_weight[them_idx] += safety_bishop_weight;

                if constexpr (Trace) {
                    // TODO: Trace;
                }
            }
        }

        return score;
    }

    template<bool Trace, Color us>
    Score evaluate_rooks(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);

        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 my_rooks = pos.get_bitboard(PieceType::ROOK, us);

        constexpr int us_idx = static_cast<int>(us) - 1;
        constexpr int them_idx = static_cast<int>(them) - 1;
       
        ec.attacked_by[us_idx][1] = 0ULL;
        int sign = us == Color::WHITE ? 1 : -1;

        Score score{};
        while (my_rooks) {
            Square sq = Bitwise::pop_lsb(my_rooks);

            ui64 attacks = AttackTables::get_rook_attacks(sq, ec.occupied_minus_rooks[us_idx]);
            ec.attacked_by_2[us_idx]  |= attacks & ec.attacked[us_idx];
            ec.attacked[us_idx]       |= attacks;
            ec.attacked_by[us_idx][1] |= attacks;

            // Open file
            if (!(my_pawns & Bitboards::file_mask(Bitboards::file_of(sq)))) {
                bool open = !(enemy_pawns & Bitboards::file_mask(Bitboards::file_of(sq)));
                score += rook_file[open];

                if constexpr (Trace) {
                    et->add_1d(M::RookFile, open, sign);
                }
            }

            // Rook seventh
            if(Bitboards::relative_rank_of(sq, us) == 6 
                && Bitboards::relative_rank_of(ec.king_sq[them_idx], us) >= 6) 
            {
                score += rook_on_seventh;

                if constexpr (Trace) {
                    et->add((size_t)M::RookOnSeventh, sign);
                }
            }

            // Mobility
            int mobility = Bitwise::count(ec.mobility_areas[us_idx] & attacks);
            score += rook_mobility[mobility];

            if constexpr (Trace) {
                et->add_1d(M::RookMobility, mobility, sign);
            }

            if ((attacks &= ec.king_areas[them_idx] & ~ec.pawn_attacks_by_2[them_idx])) {
                ec.king_attacks_count[them_idx] += Bitwise::count(attacks);
                ec.king_attackers_count[them_idx] += 1;
                ec.king_attackers_weight[them_idx] += safety_rook_weight;

                if constexpr (Trace) {
                    // TODO : Trace
                }
            }
        }
        return score;
    }

    template<bool Trace, Color us>
    Score evaluate_queens(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);

        constexpr int us_idx   = static_cast<int>(us)   - 1;
        constexpr int them_idx = static_cast<int>(them) - 1;

        ui64 my_queens = pos.get_bitboard(PieceType::QUEEN, us);
        ui64 occupancy = pos.get_total_occupancy();
        int sign = us == Color::WHITE ? 1 : -1;
        Score score{};


        ec.attacked_by[us_idx][5] = 0ULL;

        while (my_queens) {
            Square sq = Bitwise::pop_lsb(my_queens);

            ui64 attacks = AttackTables::get_queen_attacks(sq, occupancy);
            ec.attacked_by_2[us_idx]  |= attacks & ec.attacked[us_idx];
            ec.attacked[us_idx]       |= attacks;
            ec.attacked_by[us_idx][5] |= attacks;

            if (discovered_attacks(pos, sq, us)) {
                score += queen_relative_pin;

                if constexpr (Trace) {
                    et->add((size_t)M::QueenRelativePin, sign);
                }
            }

            int mobility = Bitwise::count(ec.mobility_areas[us_idx] & attacks);
            score += queen_mobility[mobility];

            if constexpr (Trace) {
                et->add_1d(M::QueenMobility, mobility, sign);
            }

            if ((attacks &= ec.king_areas[them_idx] & ~ec.pawn_attacks_by_2[them_idx])) {
                ec.king_attacks_count[them_idx]    += Bitwise::count(attacks);
                ec.king_attackers_count[them_idx]  += 1;
                ec.king_attackers_weight[them_idx] += safety_queen_weight;

                if constexpr (Trace) {
                    // TODO: Trace;
                }
            }
        }

        return score;
    }

    template<bool Trace, Color us>
    void evaluate_king_shelter(const Pos& pos, int shelter_sign, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        if (ec.pkentry) return;

        constexpr int us_idx = static_cast<int>(us) - 1;
        Square ksq = pos.get_king_square(us);
        constexpr Color them = opponent_of(us);
        int king_file = Bitboards::file_of(ksq);
        int king_rank = Bitboards::rank_of(ksq);

        ui64 my_pawns    = pos.get_bitboard(PieceType::PAWN, us);
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 pawns = my_pawns | enemy_pawns;
        
        int dist = Bitboards::king_pawn_file_distance(pawns, ksq);
        ec.pk_eval[us_idx] += king_pawn_proximity[dist];

        if constexpr (Trace) {
            //TODO: et->add_1d(M::KingPawnProximity, dist, 8, sign);
        }

        for (int f = std::max(0, king_file - 1); f <= std::min(7, king_file + 1); ++f) {
            // Friendly pawn shelter
            ui64 our_pawn = my_pawns & Bitboards::file_mask(f) & Bitboards::forward_file_mask(king_rank, us);
            int our_dist = !our_pawn ? 7 : std::abs(king_rank - Bitboards::rank_of(Bitboards::backmost(us, our_pawn)));

            ui64 their_pawn = enemy_pawns & Bitboards::file_mask(f) & Bitboards::forward_file_mask(king_rank, us);
            int their_dist = !their_pawn ? 7 : std::abs(king_rank - Bitboards::rank_of(Bitboards::backmost(us, their_pawn)));

            bool is_king_file = (f == king_file);
            ec.pk_eval[us_idx] += king_shelter[is_king_file][f][our_dist];
            ec.pk_safety[us_idx] += safety_shelter[is_king_file][our_dist];

            // pawn storm
            int blocked = (our_dist != 7 && (our_dist == their_dist - 1));
            ec.pk_eval[us_idx]   += king_storm[blocked][Bitboards::mirror_file(f)][their_dist];
            ec.pk_safety[us_idx] += safety_storm[blocked][their_dist];

            if constexpr (Trace) {
                et->add_3d(M::KingShelter, is_king_file, our_dist, f, 8, 8, shelter_sign);
                et->add_2d(M::KingStorm, blocked, their_dist, 8, shelter_sign);
                // TODO: Update
            }
        }
    }

    template<bool Trace, Color us>
    Score evaluate_kings(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);

        constexpr int us_idx   = static_cast<int>(us)   - 1;
        constexpr int them_idx = static_cast<int>(them) - 1;

        ui64 enemy_queens = pos.get_bitboard(PieceType::QUEEN, them);
        ui64 defenders = pos.get_bitboard(PieceType::PAWN  , us) |
                         pos.get_bitboard(PieceType::KNIGHT, us) |
                         pos.get_bitboard(PieceType::BISHOP, us);
        int king_sq = ec.king_sq[us_idx];

        Score score{};

        int count = Bitwise::count(defenders & ec.king_areas[us_idx]);
        score += king_defenders[count];

        if constexpr (Trace) {
            // TODO: Trace
        }

        constexpr auto to_idx = [](PieceType t) {return static_cast<int>(t); };

        if (ec.king_attackers_count[us_idx] > 1 - Bitwise::count(enemy_queens)) {
            ui64 weak = ec.attacked[them_idx]
                & ~ec.attacked_by_2[us_idx]
                & (~ec.attacked[us_idx] | ec.attacked_by[us_idx][to_idx(PieceType::QUEEN)] | ec.attacked_by[us_idx][to_idx(PieceType::KING)]);

            int scaled_attack_counts = 9.0 * ec.king_attacks_count[us_idx] / Bitwise::count(ec.king_areas[us_idx]);

            ui64 safe = ~pos.get_occupancy(them) & (~ec.attacked[us_idx] | (weak & ec.attacked_by_2[them_idx]));

            ui64 occupied       = pos.get_total_occupancy();
            ui64 knight_threats = AttackTables::get_knight_attacks(king_sq);
            ui64 bishop_threats = AttackTables::get_bishop_attacks(king_sq, occupied);
            ui64 rook_threats   = AttackTables::get_rook_attacks(king_sq, occupied);
            ui64 queen_threats  = bishop_threats | rook_threats;

            ui64 knight_checks = knight_threats & safe & ec.attacked_by[them_idx][to_idx(PieceType::KNIGHT)];
            ui64 bishop_checks = bishop_threats & safe & ec.attacked_by[them_idx][to_idx(PieceType::BISHOP)];
            ui64 rook_checks   = rook_threats   & safe & ec.attacked_by[them_idx][to_idx(PieceType::ROOK)  ];
            ui64 queen_checks  = queen_threats  & safe & ec.attacked_by[them_idx][to_idx(PieceType::QUEEN) ];

            Score safety = ec.king_attackers_weight[us_idx];

            safety += safety_attack_value * scaled_attack_counts
                + safety_weak_squares      * Bitwise::count(weak & ec.king_areas[us_idx])
                + safety_no_enemy_queens   * !enemy_queens
                + safety_safe_queen_check  * Bitwise::count(queen_checks)
                + safety_safe_rook_check   * Bitwise::count(rook_checks)
                + safety_safe_bishop_check * Bitwise::count(bishop_checks)
                + safety_safe_knight_check * Bitwise::count(knight_checks)
                + ec.pk_safety[us_idx]
                + safety_adjustment;

            if constexpr (Trace) {
                // TODO: Trace safety componenets
            }

            int mg = safety.mg, eg = safety.eg;
            score += Score(-mg * std::max(0, mg) / 720, -std::max(0, eg) / 20);

            if constexpr (Trace) {
                // TODO: trace safety score
            }         
        }

        else if constexpr (Trace) {
            // TODO reset safety terms
        }

        
        return score;
    }

    template <bool Trace, Color us>
    Score evaluate_passed(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        constexpr Color them = opponent_of(us);
        constexpr int sign = us == Color::WHITE ? 1 : -1;
        
        constexpr int us_idx   = static_cast<int>(us)   - 1;
        constexpr int them_idx = static_cast<int>(them) - 1;

        ui64 my_passed = pos.get_bitboard(PieceType::PAWN, us) & ec.passed_pawns;
        ui64 occupied = pos.get_total_occupancy();
        ui64 temp_passed = my_passed;

        Score score{};
        while (temp_passed) {
            Square sq = Bitwise::pop_lsb(temp_passed);
            int rank = Bitboards::relative_rank_of(sq, us);
            ui64 bb = AttackTables::pawn_advance<us>(1ULL << sq, 0ULL);

            bool can_advance = !(bb & occupied);
            bool safe_advance = !(bb & ec.attacked[them_idx]);
            score += passed_pawn[can_advance][safe_advance][rank];

            if constexpr (Trace) {
                et->add_3d(M::PassedPawn, can_advance, safe_advance, rank, 2, 8, sign);
            }

            if(Bitboards::several(Bitboards::forward_file_mask(sq, us) & my_passed)) continue;


            // King Proximity
            int my_dist    = Bitboards::distance(sq, ec.king_sq[us_idx  ]);
            int enemy_dist = Bitboards::distance(sq, ec.king_sq[them_idx]);

            score += my_dist    * passed_friendly_distance[rank];
            score += enemy_dist * passed_enemy_distance   [rank];

            if constexpr (Trace) {
                et->add_1d(M::PassedFriendlyDistance, rank, sign * my_dist);
                et->add_1d(M::PassedEnemyDistance, rank, sign * enemy_dist);
            }

            // Simple "Storm" Check (Is the path clear of pieces?)
            ui64 path = Bitboards::forward_file_mask(sq, us);
            bool f = !(path & (pos.get_occupancy(them) | ec.attacked[them_idx]));
            score += f * passed_safe_promo_path;

            if constexpr (Trace) {
                et->add((size_t)M::PassedSafePromoPath, sign * f);
            }
        }
        return score;
    }

    template <bool Trace>
    Score evaluate_pieces(const Pos& pos, EvalCache& ec, EvalInfo::EvalTrace* et = nullptr) {
        Score s{};
        s += evaluate_pawns  <Trace, Color::WHITE>(pos, ec, et) - evaluate_pawns  <Trace, Color::BLACK>(pos, ec, et);

        evaluate_king_shelter<Trace, Color::WHITE>(pos, 1, ec, et); evaluate_king_shelter<Trace, Color::BLACK>(pos, -1, ec, et);

        s += evaluate_knights<Trace, Color::WHITE>(pos, ec, et) - evaluate_knights<Trace, Color::BLACK>(pos, ec, et);
        s += evaluate_bishops<Trace, Color::WHITE>(pos, ec, et) - evaluate_bishops<Trace, Color::BLACK>(pos, ec, et);
        s += evaluate_rooks  <Trace, Color::WHITE>(pos, ec, et) - evaluate_rooks  <Trace, Color::BLACK>(pos, ec, et);
        s += evaluate_queens <Trace, Color::WHITE>(pos, ec, et) - evaluate_queens <Trace, Color::BLACK>(pos, ec, et);
        s += evaluate_kings  <Trace, Color::WHITE>(pos, ec, et) - evaluate_kings  <Trace, Color::BLACK>(pos, ec, et);
        s += evaluate_passed <Trace, Color::WHITE>(pos, ec, et) - evaluate_passed <Trace, Color::BLACK>(pos, ec, et);

        return s;
    }

    template <bool Trace>
	int evaluate(const Pos& pos, TT& tt, ui8 current_age, EvalInfo::EvalTrace* et = nullptr) {
        EvalCache ec{};
        ec.init(pos);

        const int sign = (pos.turn() == Color::WHITE) ? 1 : -1;
        if constexpr (!Trace) {
            ui64 key = pos.get_zobrist_key();
            auto* entry = tt.probe(key);
            if (entry && entry->static_eval_ != Constants::SCORE_NONE) {
                return entry->static_eval_;
            }
        }

        int psqt_score = pos.evaluate();
        Score total_score = evaluate_pieces<Trace>(pos, ec, et);
        
        if constexpr (!Trace) {
            if (!ec.pkentry) {
                pawns_cache.store(pos.get_pawn_key(), ec.pk_eval->mg, ec.pk_eval->eg, ec.passed_pawns, ec.pk_safety);
            }
        }

        int phase = pos.get_phase();
        int final_eval = total_score.eval(phase) + psqt_score * sign;

        if constexpr (!Trace) tt.store_eval(pos.get_zobrist_key(), static_cast<i16>(final_eval), current_age);

        return final_eval;
	}
}