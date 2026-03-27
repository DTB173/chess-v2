#pragma once

#include <utility>
#include <array>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <format>
#include <chrono>
#include <atomic>

#include "Types.h"
#include "Position.h"
#include "MoveGen.h"
#include "PSQTables.h"
#include "TranspositionTable.h"
#include "See.h"
#include "Evaluation.h"

namespace Sort {
	using namespace Types;

    constexpr int MVV_LVA[7][7] = {
        {0,  0,  0,  0,  0,  0,  0}, // No Victim
        {0, 15, 14, 13, 12, 11, 10}, // Victim Pawn
        {0, 25, 24, 23, 22, 21, 20}, // Victim Knight
        {0, 35, 34, 33, 32, 31, 30}, // Victim Bishop
        {0, 45, 44, 43, 42, 41, 40}, // Victim Rook
        {0, 55, 54, 53, 52, 51, 50}, // Victim Queen
        {0,  0,  0,  0,  0,  0,  0}  // Victim King
    };

    int score_move(Move move, const Position::Position& pos) {
        int score = 0;

        const auto& mailbox = pos.get_mailbox();
        Piece moving_piece = pos.get_mailbox()[move.from()];
        PieceType pt = moving_piece.type();

        //  Promotion bonus
        if (move.is_promo()) {
            int promo_score = (move.promotion_type() == PieceType::QUEEN) ? 120000 : 20000;
            if (move.is_capture()) promo_score += 10000;
            return promo_score;
        }

        // Capture bonus
        if (move.is_capture()) {
            PieceType victim_pt = mailbox[move.to()].type();
            if (move.is_en_passant()) victim_pt = PieceType::PAWN;
            return 80000 + MVV_LVA[static_cast<int>(victim_pt)][static_cast<int>(pt)];
        }


        int phase = pos.get_phase();

        Score s_from = PSQTables::get_piece_value(pt, moving_piece.color(), move.from());
        Score s_to = PSQTables::get_piece_value(pt, moving_piece.color(), move.to());
        score += (s_to - s_from).eval(phase);

        if (move.is_castling()) score += 8000;

        return score;
    }

    class MovePicker {
    public:
        using HistoryTable = int[2][64][64];

        MovePicker(MoveGen::MoveList& list, const Position::Position& pos, 
            const HistoryTable& history, const std::array<Move, 2>& killers = {}, Move tt_move = Move{}) : list(list) {
            for (size_t i = 0; i < list.size(); ++i) {
                // If it's the move from the TT, give it a massive bonus to ensure it's first
                Move move = list[i];
                if (move == tt_move) {
                    scores[i] = 1'000'000; // Best: TT Move 
                }
                else if (move == killers[0]) {
                    scores[i] = 9'000; // Priority Quiet 1
                }
                else if (move == killers[1]) {
                    scores[i] = 8'000; // Priority Quiet 2
                }
                else if (!move.is_capture() && !move.is_promo()) {
                    scores[i] = score_move(move, pos); // Standard: PST moves 
                    scores[i] += std::min(8000, history[static_cast<int>(pos.turn()) - 1][move.from()][move.to()]);
                }
                else {
                    scores[i] = score_move(move, pos); // Standard: PST moves 
                }
            }
        }
        Types::Move next_legal(Position::Position& pos) {
            Types::Move move;
            while ((move = next()) != NO_MOVE) {
                if (pos.is_legal(move)) {
                    return move;
                }
            }
            return NO_MOVE;
		}
    private:
		// Selection sort style picker: each call to next() finds the best remaining move
        Types::Move next() {
            if (current_idx >= list.size()) return NO_MOVE;

            int best_idx = current_idx;
            for (size_t i = current_idx + 1; i < list.size(); ++i) {
                if (scores[i] > scores[best_idx]) {
                    best_idx = i;
                }
            }

            std::swap(list[current_idx], list[best_idx]);
            std::swap(scores[current_idx], scores[best_idx]);

            return list[current_idx++];
        }
    private:
        MoveGen::MoveList& list;
        std::array<int, 256> scores;
        size_t current_idx = 0;
    };
}

namespace Search {
    using namespace Types;
    using TranspositionTable::TTEntry;
    using HistoryTable = int[2][64][64];
    TranspositionTable::TT tt(64); // 64 Mb tt

    constexpr int NO_TT_CUTOFF = 1'000'000;
    constexpr int INF = 32'001;
    constexpr int MATE_VAL = 32'000;
    constexpr int MAX_PLY = 64;
    constexpr int MAX_HISTORY = 16'384;
    constexpr int HISTORY_GRAVITY = 256;
    constexpr int QUEEN_VAL = 920;

    class Searcher {
    private:
        ui64 nodes_visited{};
        double max_time_ms{ 15'000.0 };
        std::chrono::steady_clock::time_point search_start;
        int max_depth{};
        int best_score_root{};
        int max_sel_depth{};

        Types::Move best_move_root{};
        ui8 current_age = 0;
        bool time_up = false;

        Move killer[2][MAX_PLY] = {};
        HistoryTable history = {};

        std::ostream& print_pv(std::ostream& out, Position::Position& pos) {
            int count = 0;
            Move pv_moves[24];

            while (count < 24) {
                ui64 key = pos.get_zobrist_key();
                TTEntry* entry = tt.probe(key);

                if (!entry || entry->move == NO_MOVE || entry->depth < 1) break;

                if (!pos.is_legal(entry->move)) break;

				if (entry->age != current_age) break;
                out << move_to_string(entry->move) << ' ';

                pv_moves[count] = entry->move;
                pos.make_move(entry->move);
                count++;
            }

            for (int i = 0; i < count; i++) {
                pos.undo_move();
            }

            return out;
        }
        
        int tt_lookup(ui64 zobrist, int depth, int ply, int& alpha, int& beta, Move& tt_move) {
            TTEntry* entry = tt.probe(zobrist);
            if (!entry) return NO_TT_CUTOFF;

            tt_move = entry->move;
            if (entry->depth >= depth) {

                int tt_score = tt.score_from_tt(entry->score, ply);

                if (entry->type == TranspositionTable::EXACT) return tt_score;
                if (entry->type == TranspositionTable::LOWER_BOUND) alpha = std::max(alpha, tt_score);
                if (entry->type == TranspositionTable::UPPER_BOUND) beta = std::min(beta, tt_score);
                if (alpha >= beta) return tt_score;
            }
            return NO_TT_CUTOFF;
        }

        void update_history(const Move& move, Color side, int bonus) {
            int delta = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
            int& val = history[static_cast<int>(side) - 1][move.from()][move.to()];
            val += delta - (val * std::abs(delta)) / HISTORY_GRAVITY;
            val = std::clamp(val, -MAX_HISTORY, MAX_HISTORY);
        }

        //Quiescence Search
        int quiescence(Position::Position& pos, int alpha, int beta, int ply) {
            nodes_visited++;
            if (ply > max_sel_depth) max_sel_depth = ply;

            auto* entry = tt.probe(pos.get_zobrist_key());
            if (entry && entry->depth >= 0) {  // QS depth is usually 0
                using TranspositionTable::NodeType;
                int tt_score = TranspositionTable::TT::score_from_tt(entry->score,ply);
                if (entry->type == NodeType::EXACT) return tt_score;
                if (entry->type == NodeType::LOWER_BOUND && tt_score >= beta) return tt_score;
                if (entry->type == NodeType::UPPER_BOUND && tt_score <= alpha) return tt_score;
            }

            int old_alpha = alpha;
            int eval = Eval::evaluate(pos, tt, current_age);
            if (pos.turn() == Color::BLACK) {
                eval = -eval;
            }
            // Safety: limit QS depth
            if (ply >= MAX_PLY) return eval;

            bool in_check = pos.is_in_check(pos.turn());

            int standing_pat = eval;
            if (in_check) {
                // If in check, we must ignore standing_pat and search for evasions
                standing_pat = -INF;
            }
            else {
                if (standing_pat >= beta) return beta;
                if (standing_pat > alpha) alpha = standing_pat;
            }

            if (!in_check && standing_pat + QUEEN_VAL< alpha) {
                ui64 friendly_pawns = pos.get_bitboard(PieceType::PAWN, pos.turn());
                constexpr ui64 RANK_7 = 0x00FF000000000000ULL;
                constexpr ui64 RANK_2 = 0x000000000000FF00ULL;

                ui64 pawns_near_promo = (pos.turn() == Color::WHITE)
                    ? (friendly_pawns & RANK_7)
                    : (friendly_pawns & RANK_2);
                if(!pawns_near_promo) return alpha;  // futility: even best capture can't reach alpha
            }

            MoveGen::MoveList list = in_check ? MoveGen::generate_all_moves(pos) : MoveGen::generate_captures_and_promos(pos);
            Sort::MovePicker picker(list, pos, history);
            Move move{};
            Move best_move{};
            int legal_moves{};

            while ((move = picker.next_legal(pos)) != NO_MOVE) {
                legal_moves++;
                if ((nodes_visited & 127) == 0) { // check time every 128 nodes
                    if (should_stop()) {
                        time_up = true;
                        return 0; // return last known evaluation
                    }
                }
                if (!in_check && See::see(pos, move) < 0) {
                    continue;
                }
                pos.make_move(move);
                int score = -quiescence(pos, -beta, -alpha, ply + 1);
                pos.undo_move();

                if (time_up) return 0;
                if (score >= beta) {
                    tt.store(pos.get_zobrist_key(), 0, beta, move, TranspositionTable::NodeType::LOWER_BOUND, ply, current_age, eval);
                    return beta;
                } 
                if (score > alpha) {
                    alpha = score;
                    best_move = move;
                }
            }

            // Checkmate detection
            if (in_check && legal_moves == 0) return -MATE_VAL + ply;

            // TT store
            using TranspositionTable::NodeType;
            NodeType bound = (alpha <= old_alpha) ? NodeType::UPPER_BOUND :
                (alpha >= beta) ? NodeType::LOWER_BOUND : NodeType::EXACT;
            tt.store(pos.get_zobrist_key(), 0, alpha, best_move, bound, ply, current_age, eval);
            return alpha;
        }

        // Main Negamax Search
        int negamax(Position::Position& pos, int depth, int ply, int alpha, int beta, bool allowed_null) {
            // 1. Terminal / Draw checks
            if (ply > 0) {
                if (pos.is_draw()) return 0;
            }

            // Check time/interrupts periodically
            if ((nodes_visited++ & 2047) == 0) {
                if (should_stop()) time_up = true;
            }
            if (time_up) return 0;

            // 2. Transposition Table Lookup
            Move tt_move = NO_MOVE;
            int tt_score = tt_lookup(pos.get_zobrist_key(), depth, ply, alpha, beta, tt_move);
            if (tt_score != NO_TT_CUTOFF && ply > 0) return tt_score;

            // 3. Quiescence at leaf
            if (depth <= 0) return quiescence(pos, alpha, beta, ply);

            // Safety cutoff
            int static_eval = Eval::evaluate(pos, tt, current_age);
            if (ply >= 64) return static_eval;

            bool in_check = pos.is_in_check(pos.turn());
            
            // Note: Ensure your evaluate() handles side-to-move correctly

            // 4. Null Move Pruning
            if (allowed_null && depth >= 3 && !in_check && ply > 0 && pos.has_non_pawn_material(pos.turn())) {
                int R = 2 + depth / 3;
                pos.make_null_move();
                int score = -negamax(pos, depth - 1 - R, ply + 1, -beta, -beta + 1, false);
                pos.undo_null_move();
                if (score >= beta) return beta;
            }
            // 5. Move Generation
            std::array<Move, 2> ply_killers = { killer[0][ply], killer[1][ply] };
            MoveGen::MoveList list = MoveGen::generate_all_moves(pos);
            Sort::MovePicker picker(list, pos, history, ply_killers, tt_move);
            Move move;

            int legal_moves = 0;
            int best_score = -INF;
            Move best_move = NO_MOVE;
            int old_alpha = alpha;

            // 6. Loop
            while ((move = picker.next_legal(pos)) != NO_MOVE) {
                legal_moves++;

                if ((nodes_visited & 127) == 0) { // check time every 128 nodes
                    if (should_stop()) {
                        time_up = true;
                        return 0; // return last known evaluation
                    }
                }
                pos.make_move(move);

                int score;

                // Principal Variation Search Logic
                if (legal_moves == 1) {
                    // First move: Full Window Search
                    score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                }
                else {
                    int reduction = 0;

                    if (depth >= 3 && !in_check && legal_moves >= 4) {
                        // Base reduction
                        reduction = 1 + (int)(std::log(depth) * std::log(legal_moves) / 2.5);

                        // Deeper reductions for late moves
                        if (depth >= 5 && legal_moves >= 8)  reduction += 1;
                        if (depth >= 8 && legal_moves >= 12) reduction += 1;

                        // Reduce more if history is low (bad move)
                        if (!move.is_capture() && !move.is_promo()) {
                            int hist = history[static_cast<int>(pos.turn()) - 1][move.from()][move.to()];
                            if (hist < 0)         reduction += 1;
                            else if (hist > 2000) reduction -= 1;
                        }

                        // Never reduce too much
                        reduction = std::min(reduction, depth - 2);
                    }

                    // Late moves: Zero Window Search
                    score = -negamax(pos, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, true);

                    // If the move beat alpha, we must re-search with full window
                    if (score > alpha) {
                        score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                    }
                }

                pos.undo_move();

                if (time_up) return 0;

                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                    if (ply == 0) best_move_root = move;

                    if (score > alpha) {
                        alpha = score;
                        if (score >= beta) break; // Beta Cutoff
                    }
                }
            }

            // 7. Checkmate / Stalemate
            if (legal_moves == 0) {
                return in_check ? -MATE_VAL + ply : 0;
            }

            // 8. TT Store
            if (!time_up) {
                using TranspositionTable::NodeType;
                NodeType bound = (best_score <= old_alpha) ? NodeType::UPPER_BOUND :
                    (best_score >= beta) ? NodeType::LOWER_BOUND : NodeType::EXACT;

                tt.store(pos.get_zobrist_key(), depth, best_score, best_move, bound, ply, current_age, static_eval);

                // Update Heuristics on Cutoff
                if (best_score >= beta && !best_move.is_capture()) {
                    update_history(best_move, pos.turn(), 200 * depth);
                    killer[1][ply] = killer[0][ply];
                    killer[0][ply] = best_move;
                }
            }

            return best_score;
        }

        void check_time() {
            if ((nodes_visited & 1023) == 0) {
                if (should_stop()) {
                    time_up = true;
                }
            }
        }
    public:
        static inline std::atomic <bool> stop_signal;

        void set_max_time(double seconds) {
            max_time_ms = seconds * 1000.0;
        }

        bool should_stop() {
            if (time_up) return true;

            if (stop_signal.load(std::memory_order_relaxed)) {
                time_up = true;
                return true;
            }

            double elapsed_ms = time_elapsed_ms(search_start);
            constexpr int overhead = 50;

            if (elapsed_ms >= (max_time_ms - overhead)) {
                time_up = true;
                return true;
            }

            return false;
        }

        inline double time_elapsed_ms(std::chrono::steady_clock::time_point start)
        {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<double, std::milli>(now - start).count();
        }



        Move start_search(Position::Position& pos, int target_depth = 64, double max_time_sec = 1500.0)
        {
            best_move_root = NO_MOVE;
            nodes_visited = 0;
            max_sel_depth = 0;
            current_age++;
            time_up = false;

            max_time_ms = max_time_sec * 1000.0;
            search_start = std::chrono::steady_clock::now();
            time_up = false;

            const double soft_limit_ms = max_time_ms * 0.85;

            Move best_so_far = NO_MOVE;
            //int best_score_so_far = -INF;

            int alpha = -INF;
            int beta = +INF;
            int window = 40;

            for (int depth = 1; depth <= target_depth; ++depth)
            {
                // Early exit before starting expensive iteration
                if ((time_elapsed_ms(search_start) >= soft_limit_ms) && depth > 1) break;

                int score = negamax(pos, depth, 0, alpha, beta, true);

                if (score <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = std::max(-INF, alpha - window);
                    window *= 2;
                }
                else if (score >= beta) {
                    alpha = (alpha + beta) / 2;
                    beta = std::min(INF, beta + window);
                    window *= 2;
                }

                alpha = score - window;
                beta = score + window;

                if (time_up) break;

                // Only accept completed iteration results
                if (best_move_root != NO_MOVE)
                {
                    best_so_far = best_move_root;
                    //best_score_so_far = score;
                }

                // Calculate stats
                const auto current_time = std::chrono::steady_clock::now();
                const double elapsed_sec = std::chrono::duration<double>(current_time - search_start).count();
                const uint64_t nps = (elapsed_sec > 0.001)
                    ? static_cast<uint64_t>(nodes_visited / elapsed_sec)
                    : 0;

                // Format score for UCI (mate / cp)
                std::string score_str;
                if (std::abs(score) >= MATE_VAL - 100) {
                    int dist = (MATE_VAL - std::abs(score) + 1) / 2;
                    score_str = std::string("mate ") + (score > 0 ? "" : "-") + std::to_string(dist);
                }
                else {
                    score_str = "cp " + std::to_string(score);
                }

                // UCI info line
                std::cout << std::format(
                    "info depth {} seldepth {} score {} nodes {} nps {} time {} pv ",
                    depth,
                    max_sel_depth,
                    score_str,
                    nodes_visited,
                    nps,
                    static_cast<int>(elapsed_sec * 1000)
                );
            
                print_pv(std::cout, pos) << '\n' << std::flush;

                // If time ran out during search we still have previous best move
                if (should_stop()) break;

            }
            return best_so_far;
        }
    };
}