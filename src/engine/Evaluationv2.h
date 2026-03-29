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

    constexpr inline Score king_shelter[2][8] = {
        { {   0,    0}, {  -5,   -5}, {  10,   10}, {  25,   30}, {  40,   45}, {  55,   60}, {  65,   70}, {  70,   75} },
        { {   0,    0}, {   5,    5}, {  20,   25}, {  45,   55}, {  70,   85}, {  90,  110}, { 105,  130}, { 115,  145} }
    };

    constexpr inline Score king_storm[2][8] = {
        { {   0,    0}, {   5,    0}, {  15,    5}, {  25,   10}, {  35,   15}, {  45,   20}, {  55,   25}, {  60,   30} },
        { {   0,    0}, {  10,    5}, {  25,   15}, {  40,   25}, {  60,   40}, {  80,   55}, { 100,   75}, { 110,   90} }
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
        {-105, -140}, { -45, -115}, { -20,  -35}, {  -5,    0},
        {  -8,    5}, {   5,   15}, {  10,   35}, {  20,   40},
        {  30,   35}, {  45,   15}
    };

    constexpr inline Score bishop_mobility[14] = {
        {-100, -185}, { -45, -125}, { -15,  -55}, {  -5,  -15},
        {   5,    0}, {  15,   20}, {  15,   35}, {  20,   40},
        {  20,   50}, {  25,   50}, {  25,   50}, {  50,   30},
        {  55,   45}, {  85,    0}
    };

    constexpr inline Score rook_mobility[15] = {
        {-125, -150}, { -55, -125}, { -25,  -85}, { -10,  -30},
        { -10,    0}, { -10,   25}, { -10,   40}, {  -5,   45},
        {   5,   50}, {   5,   55}, {  10,   65}, {  20,   70},
        {  20,   75}, {  35,   60}, { 100,   15}
    };

    constexpr inline Score queen_mobility[28] = {
        {-110, -275}, {-255, -400}, {-125, -230}, { -45, -235},
        { -20, -175}, { -10,  -85}, {   0,  -35}, {   0,    0},
        {  10,   10}, {  10,   30}, {  15,   35}, {  15,   55},
        {  20,   45}, {  25,   55}, {  20,   60}, {  20,   65},
        {  25,   60}, {  15,   65}, {  15,   65}, {  18,   50},
        {  25,   30}, {  40,   10}, {  35,  -10}, {  30,  -30},
        {  10,  -45}, {   5,  -80}, { -40,  -30}, { -25,  -50}
    };

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

    PawnTable::PawnTable pawns_cache(16);

    using M = EvalInfo::Mapper;
    ui64 discovered_attacks(const Position::Position& pos, Square sq, Color us, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);

        ui64 occupied = pos.get_total_occupancy();

        ui64 rook_attacks = get_rook_attacks(sq, occupied);
        ui64 bishop_attacks = get_bishop_attacks(sq, occupied);

        ui64 enemy_rooks = pos.get_bitboard(PieceType::ROOK, them) & ~rook_attacks;
        ui64 enemy_bishops = pos.get_bitboard(PieceType::BISHOP, them) & ~bishop_attacks;

        return (enemy_rooks & get_rook_attacks(sq, occupied & ~rook_attacks))
            | (enemy_bishops & get_bishop_attacks(sq, occupied & ~bishop_attacks));
    }

    Score evaluate_pawns(const Position::Position& pos, Color us, ui64& passed_pawns, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);
        int forward = (us == Color::WHITE) ? 8 : -8;

        Score score = Score(0, 0);
        ui64 my_pawns    = pos.get_bitboard(PieceType::PAWN, us);
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);

        int sign = us == Color::WHITE ? 1 : -1;
       
        ui64 temp = my_pawns;
        while (temp) {
            Square sq = Bitwise::pop_lsb(temp);
            int rank  = Bitboards::relative_rank_of(sq, us);
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
                Bitwise::set_bit(passed_pawns, sq);
            }
            // Candidate Passed Pawn
            else if (!leftovers && Bitwise::count(push_support) > Bitwise::count(push_threats)) {
                bool supported = (Bitwise::count(support) >= Bitwise::count(threats));
                score += pawn_passed_candidate[supported][rank];

                if (et) {
                    et->add_2d(M::PassedPawnCandidate, supported, relative_rank, 8 , sign);
            }
            }

            // Isolated Pawn
            if (neighbours == 0 && threats == 0) {
                score += pawn_isolated[file];
                if (et) {
                    et->add_1d(M::PawnIsolated, file, sign);
            }
            }

            // Stacked / Doubled Pawns
            if (Bitwise::count(Bitboards::file_mask(file) & my_pawns) > 1) {
                bool can_undouble = (stoppers && (threats || neighbours))
                    || (stoppers & ~Bitboards::forward_file_mask(sq, us));
                bool stacked_flag = !can_undouble;
                score += pawn_stacked[stacked_flag][file];
                if (et) {
                    et->add_2d(M::PawnStacked, stacked_flag, file, 8, sign);
            }
            }

            // Backward Pawn
            if (neighbours && push_threats && !backup) {
                bool can_be_attacked_safely = !(Bitboards::file_mask(file) & enemy_pawns);
                bool backwards_flag = !can_be_attacked_safely;
                score += pawn_backward[backwards_flag][rank];
                if (et) {
                    et->add_2d(M::PawnBackwards, backwards_flag, relative_rank, 8, sign);
            }
            }
            // Connected Pawns
            else if ((AttackTables::get_connected_mask(sq, us) & my_pawns)) {
                int relative_sq32 = Bitboards::relative_square32(sq, us);
                score += pawn_connected32[relative_sq32];
                if (et) {
                    et->add_1d(M::PawnConnected, relative_sq32, sign);
                }
            }
        }

        return score;
    }

    Score evaluate_pawn_structure(const Position::Position& pos, EvalInfo::EvalTrace* et = nullptr) {
        if (!et) {
        if (auto* pentry = pawns_cache.probe(pos.get_pawn_key()); pentry) {
            return { pentry->score_mg, pentry->score_eg };
        }
        }

        ui64 passed_pawns{};
        Score white_score = evaluate_pawns(pos, Color::WHITE, passed_pawns, et);
        Score black_score = evaluate_pawns(pos, Color::BLACK, passed_pawns, et);

        Score s = white_score - black_score;
        pawns_cache.store(pos.get_pawn_key(), s.mg, s.eg, passed_pawns);
        return s;
    }

    Score evaluate_knights(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 temp = pos.get_bitboard(PieceType::KNIGHT, us);

        Square my_ksq = pos.get_king_square(us);
        Square enemy_ksq = pos.get_king_square(them);

        int sign = us == Color::WHITE ? 1 : -1;

        Score score{};

        while (temp) {
            Square sq = Bitwise::pop_lsb(temp);

            // Outpost: Only if it can't be chased by enemy pawns
            if (Bitwise::test_bit(Bitboards::get_outpost_rank_mask(us), sq)
                && !(Bitboards::outpost_square_mask(sq, us) & enemy_pawns))
            {
                bool outside = (Bitboards::file_of(sq) < 2 || Bitboards::file_of(sq) > 5);
                bool defended = (AttackTables::get_pawn_attacks(sq, them) & my_pawns);
                score += knight_outpost[outside][defended];

                if (et) {
                    et->add_2d(M::KnightOutpost, outside, defended, 2, sign);
            }
            }

            // Knight behind pawn 
            Square behind = (us == Color::WHITE) ? sq - 8 : sq + 8;
            if (behind >= 0 && behind < 64 && (my_pawns & (1ULL << behind))) {
                score += knight_behind_pawn;

                if (et) {
                    et->add((size_t)M::KnightBehindPawn, sign);
            }
            }

            // Proximity: Chebyshev distance
            int dist = std::min(Bitboards::distance(sq, my_ksq), Bitboards::distance(sq, enemy_ksq));
            if (dist >= 4) {
                int d = std::min(dist - 4, 3);
                score += knight_in_narnia[d]; // Clamp to table size

                if (et) {
                    et->add_1d(M::KnightInNarnia, d, sign);
            }
            }

            // 4. Mobility
            ui64 moves = AttackTables::get_knight_attacks(sq) & ~pos.get_occupancy(us);
            int mobility = Bitwise::count(moves);
            score += knight_mobility[mobility];

            if (et) {
                et->add_1d(M::KnightMobility, mobility, sign);
        }
        }
        return score;
    }

    Score evaluate_bishops(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);

        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 my_bishops = pos.get_bitboard(PieceType::BISHOP, us);
        ui64 occupancy = pos.get_total_occupancy();

        int sign = us == Color::WHITE ? 1 : -1;

        // Bishop pair bonus
        Score score{};
        if (Bitwise::count(my_bishops) >= 2) {
            score += bishop_pair;

            if (et) {
                et->add((size_t)M::BishopPair, sign);
            }
        }
        while (my_bishops) {
            Square sq = Bitwise::pop_lsb(my_bishops);

            // Outpost
            if (Bitwise::test_bit(Bitboards::get_outpost_rank_mask(us), sq)
                && !(Bitboards::outpost_square_mask(sq, us) & enemy_pawns))
            {
                bool outside = (Bitboards::file_of(sq) == 0 || Bitboards::file_of(sq) == 7);
                bool defended = (AttackTables::get_pawn_attacks(sq, them) & my_pawns);
                score += bishop_outpost[outside][defended];

                if (et) {
                    et->add_2d(M::BishopOutpost, outside, defended, 2, sign);
                }
            }

            // Bishop behind pawn
            Square behind = (us == Color::WHITE) ? sq - 8 : sq + 8;
            if (behind >= 0 && behind < 64 && (my_pawns & (1ULL << behind))) {
                score += bishop_behind_pawn;

                if (et) {
                    et->add((size_t)M::BishopBehindPawn, sign);
                }
            }

            // Center control
            ui64 diagonal_attacks = get_bishop_attacks(sq, occupancy);
            if (((Bitboards::LONG_DIAGONALS & ~Bitboards::CENTER_SQUARES) & (1ULL << sq))
                && Bitwise::count(diagonal_attacks & Bitboards::CENTER_SQUARES) >= 2) {
                score += bishop_long_diagonal;

                if (et) {
                    et->add((size_t)M::BishopLongDiagional, sign);
                }
            }

            // Mobility
            int mobility = Bitwise::count(get_bishop_attacks(sq, occupancy) & ~pos.get_occupancy(us));
            score += bishop_mobility[mobility];

            if (et) {
                et->add_1d(M::BishopMobility, mobility, sign);
            }
        }

        return score;
    }

    Score evaluate_rooks(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);

        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);
        ui64 my_pawns = pos.get_bitboard(PieceType::PAWN, us);
        ui64 my_rooks = pos.get_bitboard(PieceType::ROOK, us);
        ui64 occupancy = pos.get_total_occupancy();

        int sign = us == Color::WHITE ? 1 : -1;

        Score score{};
        while (my_rooks) {
            Square sq = Bitwise::pop_lsb(my_rooks);

            // Open file
            if (!(my_pawns & Bitboards::file_mask(Bitboards::file_of(sq)))) {
                bool open = !(enemy_pawns & Bitboards::file_mask(Bitboards::file_of(sq)));
                score += rook_file[open];

                if (et) {
                    et->add_1d(M::RookFile, open, sign);
                }
            }

            // Rook seventh
            bool enemy_king_back = Bitboards::relative_rank_of(pos.get_king_square(them), us) >= 6;
            bool enemy_pawns_there = enemy_pawns & Bitboards::rank_mask(Bitboards::relative_rank_of(sq, us));

            if (Bitboards::relative_rank_of(sq, us) == 6 && (enemy_king_back || enemy_pawns_there)) {
                score += rook_on_seventh;

                if (et) {
                    et->add((size_t)M::RookOnSeventh, sign);
                }
            }

            // Mobility
            int mobility = Bitwise::count(get_rook_attacks(sq, occupancy) & ~pos.get_occupancy(us));
            score += rook_mobility[mobility];

            if (et) {
                et->add_1d(M::RookMobility, mobility, sign);
            }
        }
        return score;
    }

    Score evaluate_queens(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        ui64 my_queens = pos.get_bitboard(PieceType::QUEEN, us);
        ui64 occupancy = pos.get_total_occupancy();
        int sign = us == Color::WHITE ? 1 : -1;
        Score score{};

        while (my_queens) {
            Square sq = Bitwise::pop_lsb(my_queens);

            if (discovered_attacks(pos, sq, us)) {
                score += queen_relative_pin;

                if (et) {
                    et->add((size_t)M::QueenRelativePin, sign);
                }
            }

            int mobility = Bitwise::count(get_queen_attacks(sq, occupancy) & ~pos.get_occupancy(us));
            score += queen_mobility[mobility];

            if (et) {
                et->add_1d(M::QueenMobility, mobility, sign);
            }
        }

        return score;
    }

    Score evaluate_king_shelter(const Position::Position& pos, Color us, int shelter_sign, EvalInfo::EvalTrace* et = nullptr) {
        Square ksq = pos.get_king_square(us);
        Color them = opponent_of(us);
        int king_file = Bitboards::file_of(ksq);

        Score score{};
        for (int f = std::max(0, king_file - 1); f <= std::min(7, king_file + 1); ++f) {
            bool is_king_file = (f == king_file);

            // Friendly pawn shelter
            ui64 our_pawns = pos.get_bitboard(PieceType::PAWN, us) & Bitboards::file_mask(f);
            int our_dist = 7;
            if (our_pawns) {
                Square closest = (us == Color::WHITE) ? Bitboards::backmost(us, our_pawns)
                    : Bitboards::frontmost(us, our_pawns);
                our_dist = std::abs(Bitboards::rank_of(ksq) - Bitboards::rank_of(closest));
            }

            // Enemy pawn storm
            ui64 their_pawns = pos.get_bitboard(PieceType::PAWN, them) & Bitboards::file_mask(f);
            int their_dist = 7;
            if (their_pawns) {
                Square closest = (us == Color::WHITE) ? Bitboards::frontmost(them, their_pawns)
                    : Bitboards::backmost(them, their_pawns);
                their_dist = std::abs(Bitboards::rank_of(ksq) - Bitboards::rank_of(closest));
            }

            bool blocked = (our_dist != 7 && our_dist == their_dist - 1);

            score += king_shelter[is_king_file][our_dist];
            score += king_storm[blocked][their_dist];

            if (et) {
                et->add_2d(M::KingShelter, is_king_file, our_dist, 8, shelter_sign);
                et->add_2d(M::KingStorm, blocked, their_dist, 8, shelter_sign);
            }
        }

        return score;
    }

    int evaluate_king_attack(const Position::Position& pos, Color us, int sign, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);
        Square ksq = pos.get_king_square(us);
        ui64 zone = Bitboards::king_area(ksq);
        ui64 occupancy = pos.get_total_occupancy();
        
        int attack_weight = 0;
        int enemy_material = 0;
        ui64 queen_bb = pos.get_bitboard(PieceType::QUEEN, them);
        enemy_material += 9 * Bitwise::count(queen_bb);
        while (queen_bb) {
            Square queen_sq = Bitwise::pop_lsb(queen_bb);
            ui64 queen_attacks = get_queen_attacks(queen_sq, occupancy);
            attack_weight += 5 * ((queen_attacks & zone) != 0);
        }

        ui64 rook_bb = pos.get_bitboard(PieceType::ROOK, them);
        enemy_material += 5 * Bitwise::count(rook_bb);
        while (rook_bb) {
            Square rook_sq = Bitwise::pop_lsb(rook_bb);
            ui64 rook_attacks = get_rook_attacks(rook_sq, occupancy);
            attack_weight += 4 * ((rook_attacks & zone) != 0);
        }

        ui64 bishop_bb = pos.get_bitboard(PieceType::BISHOP, them);
        enemy_material += 3 * Bitwise::count(bishop_bb);
        while (bishop_bb) {
            Square bishop_sq = Bitwise::pop_lsb(bishop_bb);
            ui64 bishop_attacks = get_bishop_attacks(bishop_sq, occupancy);
            attack_weight += 3 * ((bishop_attacks & zone) != 0);
            }

        ui64 knight_bb = pos.get_bitboard(PieceType::KNIGHT, them);
        enemy_material += 3 * Bitwise::count(knight_bb);
        while (knight_bb) {
            Square knight_sq = Bitwise::pop_lsb(knight_bb);
            ui64 knight_attacks = get_knight_attacks(knight_sq);
            attack_weight += 3 * ((knight_attacks & zone) != 0);
        }

        if (attack_weight < 12) return 0;

        int index = std::clamp((attack_weight * attack_weight) / 140, 0, 99);

        int base_score = safety_table[index].mg;

        int scale = (enemy_material > 35) ? 128
            : (enemy_material > 22) ? 64 + (enemy_material - 22) * 4
            : 32;

        int final_score = base_score * scale / 128;
        if (et) {
            et->add_1d(M::SafetyTable, index, sign * scale / 128);
        }

        return final_score;
    }

    Score evaluate_kings(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        int base_sign = (us == Color::WHITE) ? 1 : -1;
        int shelter_sign = base_sign * 2;

        Score shelter = evaluate_king_shelter(pos, us, shelter_sign, et);
        int attack = evaluate_king_attack(pos, us, base_sign, et);

        return shelter * 2 + Score(attack, attack / 3);
    }

    Score evaluate_passed(const Position::Position& pos, Color us, EvalInfo::EvalTrace* et = nullptr) {
        Color them = opponent_of(us);

        auto* pentry = pawns_cache.probe(pos.get_pawn_key());
        if (!pentry) return {};
        
        ui64 my_passed = pentry->passed_pawns & pos.get_bitboard(PieceType::PAWN, us);
        ui64 temp_passed = my_passed;
        ui64 occupied = pos.get_total_occupancy();
        ui64 enemy_pawns = pos.get_bitboard(PieceType::PAWN, them);

        int sign = us == Color::WHITE ? 1 : -1;
        Score score{};
        while (temp_passed) {
            Square sq = Bitwise::pop_lsb(temp_passed);
            int rank = Bitboards::relative_rank_of(sq, us);

            Square next_sq = (us == Color::WHITE) ? sq + 8 : sq - 8;
            bool can_advance = !(occupied & (1ULL << next_sq));
            bool safe_advance = can_advance && !(AttackTables::get_pawn_attacks(next_sq, us) & enemy_pawns);

            score += passed_pawn[can_advance][safe_advance][rank];

            if (et) {
                et->add_3d(M::PassedPawn, can_advance, safe_advance, rank, 2, 8, sign);
            }

            // King Proximity
            int my_dist = Bitboards::distance(sq, pos.get_king_square(us));
            int enemy_dist = Bitboards::distance(sq, pos.get_king_square(them));

            score += my_dist * passed_friendly_distance[rank];
            score += enemy_dist * passed_enemy_distance[rank];

            if (et) {
                et->add_1d(M::PassedFriendlyDistance, rank, sign * my_dist);
                et->add_1d(M::PassedEnemyDistance, rank, sign * enemy_dist);
            }

            // Simple "Storm" Check (Is the path clear of pieces?)
            ui64 path = Bitboards::forward_file_mask(sq, us) & Bitboards::file_mask(Bitboards::file_of(sq));
            if (!(path & occupied)) {
                score += passed_safe_promo_path;

                if (et) {
                    et->add((size_t)M::PassedSafePromoPath, sign);
                }
            }
        }
        return score;
    }

    Score evaluate_pieces(const Position::Position& pos, EvalInfo::EvalTrace* et = nullptr) {
        Score s{};
        s += evaluate_pawn_structure(pos, et);
        s += evaluate_knights(pos, Color::WHITE, et) - evaluate_knights(pos, Color::BLACK, et);
        s += evaluate_bishops(pos, Color::WHITE, et) - evaluate_bishops(pos, Color::BLACK, et);
        s += evaluate_rooks  (pos, Color::WHITE, et) - evaluate_rooks  (pos, Color::BLACK, et);
        s += evaluate_queens (pos, Color::WHITE, et) - evaluate_queens (pos, Color::BLACK, et);
        s += evaluate_kings  (pos, Color::WHITE, et) - evaluate_kings  (pos, Color::BLACK, et);
        s += evaluate_passed (pos, Color::WHITE, et) - evaluate_passed (pos, Color::BLACK, et);

        return s;
    }

	int evaluate(const Position::Position& pos, TT& tt, ui8 current_age, EvalInfo::EvalTrace* et = nullptr) {
        if (!et) {
		ui64 key = pos.get_zobrist_key();
		auto* entry = tt.probe(key);
		if (entry && entry->static_eval != Constants::SCORE_NONE) {
			return entry->static_eval;
		}
        }

        int phase = pos.get_phase();
        int psqt_score = pos.evaluate();
        Score total_score = evaluate_pieces(pos, et);
 
        int final_eval = total_score.eval(phase) + psqt_score;

        if(!et) tt.store_eval(pos.get_zobrist_key(), static_cast<i16>(final_eval), current_age);
        if (et) return final_eval * (pos.turn() == Color::WHITE) ? 1 : -1;
        return final_eval;
	}
}