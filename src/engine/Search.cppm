module;
#include <utility>
#include <array>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <format>
#include <chrono>

export module Search;
import Position;
import Types;
import MoveGen;
import PSQTables;
import TranspositionTable;

export namespace Sort {
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
        Piece moving_piece = mailbox[move.from()];

        // Capture bonus
        if (move.is_capture()) {
            int victim = static_cast<int>(mailbox[move.to()].type());
            int attacker = static_cast<int>(mailbox[move.from()].type());

            if (move.is_en_passant()) victim = 1; // Pawn

            // MVV_LVA gives a score between 10 and 55
            score += 10000 + MVV_LVA[victim][attacker];
        }

		//  Promotion bonus
        if (move.is_promo()) {
			// Promote to queen is best
            if (move.promotion_type() == Types::PieceType::QUEEN) {
                score += 10000;
            }
            else {
                score += 1000; // Under-promotions are less interesting
            }
        }

        if (!move.is_capture() && !move.is_promo()) {
            // We use Midgame tables for sorting usually
            // Flip the square for Black to match White-relative tables
            int from_idx = (moving_piece.color() == Types::Color::WHITE) ? move.from() : (move.from() ^ 56);
            int to_idx = (moving_piece.color() == Types::Color::WHITE) ? move.to() : (move.to() ^ 56);

            // Bonus = Value of new square - Value of old square
            int pst_diff = PSQTables::PSQ_TABLES[static_cast<int>(moving_piece.type()) - 1][to_idx].mg
                            - PSQTables::PSQ_TABLES[static_cast<int>(moving_piece.type()) - 1][from_idx].mg;

            score += pst_diff;
        }

        return score;
    }

    class MovePicker {
    public:
        MovePicker(MoveGen::MoveList& list, const Position::Position& pos, Move tt_move = Move{}) : list(list) {
            for (int i = 0; i < list.size(); ++i) {
                // If it's the move from the TT, give it a massive bonus to ensure it's first
                if (list[i] == tt_move) {
                    scores[i] = 1000000;
                }
                else {
                    scores[i] = score_move(list[i], pos);
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
            for (int i = current_idx + 1; i < list.size(); ++i) {
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
        int current_idx = 0;
    };
}

export namespace Search {
    using namespace Types;
    using TranspositionTable::TTEntry;
    TranspositionTable::TranspositionTable tt(64);

    constexpr int NO_TT_CUTOFF = 1000000;

    // Safety check: Don't use standard INT_MAX to avoid overflows
    constexpr int INF = 32001;
    constexpr int MATE_VAL = 32000;
    constexpr int MAX_PLY = 64;

    class Searcher {
    private:
        int max_depth{};
        Types::Move best_move_root{};
        int best_score_root{};
        ui64 nodes_visited{};
        int max_sel_depth{};

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

        //Quiescence Search
        int quiescence(Position::Position& pos, int alpha, int beta, int ply) {
            nodes_visited++;
            if (pos.is_draw()) return 0;
            if (ply > max_sel_depth) max_sel_depth = ply;

            // Safety: limit QS depth
            if (ply >= MAX_PLY) return pos.evaluate();

            int standing_pat = pos.evaluate();
            if (ply >= MAX_PLY) return standing_pat;

            bool in_check = pos.is_in_check(pos.turn());

            if (in_check) {
                // If in check, we must ignore standing_pat and search for evasions
                standing_pat = -INF;
            }
            else {
                if (standing_pat >= beta) return beta;
                if (standing_pat > alpha) alpha = standing_pat;
            }

            MoveGen::MoveList list = in_check ? MoveGen::generate_all_moves(pos) : MoveGen::generate_captures(pos);
            Sort::MovePicker picker(list, pos);
            Move move;
            int legal_moves = 0;

            while ((move = picker.next_legal(pos)) != NO_MOVE) {
                legal_moves++;
                pos.make_move(move);
                int score = -quiescence(pos, -beta, -alpha, ply + 1);
                pos.undo_move();

                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }

            // Checkmate detection in Q-Search
            if (in_check && legal_moves == 0) return -MATE_VAL + ply;

            return alpha;
        }

        // Main Negamax Search
        int negamax(Position::Position& pos, int depth, int ply, int alpha, int beta, bool allowed_null) {
            // 1. Terminal / Draw checks
            if (pos.is_draw()) return 0;
            if (ply >= 64) return pos.evaluate(); // Safety cutoff

            // 2. Transposition Table
            Move tt_move = NO_MOVE;
            int tt_score = tt_lookup(pos.get_zobrist_key(), depth, ply, alpha, beta, tt_move);
            if (tt_score != NO_TT_CUTOFF && ply > 0) return tt_score;

            // 3. Quiescence at leaf
            if (depth <= 0) return quiescence(pos, alpha, beta, ply);

            nodes_visited++;
            bool in_check = pos.is_in_check(pos.turn());

            // 4. Null Move Pruning          
            if (allowed_null && depth >= 3 && !in_check && ply > 0 && pos.has_non_pawn_material(pos.turn())) {
                pos.make_null_move();
                // Search with reduced depth and zero window (beta-1, beta)
                int score = -negamax(pos, depth - 3, ply + 1, -beta, -beta + 1, false);
                pos.undo_null_move();
                if (score >= beta) return beta; // Cutoff
            }

            // 5. Move Generation
            MoveGen::MoveList list = MoveGen::generate_all_moves(pos);
            Sort::MovePicker picker(list, pos, tt_move);
            Move move;

            int legal_moves = 0;
            int best_score = -INF;
            Move best_move = NO_MOVE;
            int old_alpha = alpha;

            // 6. Loop
            while ((move = picker.next_legal(pos)) != NO_MOVE) {
                legal_moves++;
                pos.make_move(move);

                int score;

                // Principal Variation Search Logic
                if (legal_moves == 1) {
                    // First move: Full Window Search
                    score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                }
                else {
                    // Late moves: Zero Window Search
                    score = -negamax(pos, depth - 1, ply + 1, -alpha - 1, -alpha, true);

                    // If the move beat alpha, we must re-search with full window
                    if (score > alpha && score < beta) {
                        score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                    }
                }

                pos.undo_move();

                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                    if (ply == 0) best_move_root = move;
                }

                if (score > alpha) {
                    alpha = score;
                }

                if (alpha >= beta) break; // Beta Cutoff
            }

            // 7. Checkmate / Stalemate
            if (legal_moves == 0) {
                return in_check ? -MATE_VAL + ply : 0;
            }

            // 8. TT Store
            using TranspositionTable::NodeType;
            NodeType bound = (best_score <= old_alpha) ? NodeType::UPPER_BOUND :
                (best_score >= beta) ? NodeType::LOWER_BOUND : NodeType::EXACT;

            tt.store(pos.get_zobrist_key(), depth, tt.score_to_tt(best_score, ply), best_move, bound, ply);

            return best_score;
        }

    public:
        Move start_search(Position::Position& pos, int target_depth) {
            best_move_root = NO_MOVE;
            nodes_visited = 0;
            max_sel_depth = 0;

            auto start_time = std::chrono::high_resolution_clock::now();

            for (int depth = 1; depth <= target_depth; ++depth) {
                int score = negamax(pos, depth, 0, -INF, INF, true);

                auto current_time = std::chrono::high_resolution_clock::now();
                auto duration = current_time - start_time;
                double seconds = std::chrono::duration<double>(duration).count();
                uint64_t nps = (seconds > 0.001) ? static_cast<uint64_t>(nodes_visited / seconds) : 0;

                std::string score_str;
                if (score > MATE_VAL - 100) score_str = "mate " + std::to_string((MATE_VAL - score + 1) / 2);
                else if (score < -MATE_VAL + 100) score_str = "mate -" + std::to_string((MATE_VAL + score) / 2);
                else score_str = "cp " + std::to_string(score);

                std::cout << std::format("info depth {} seldepth {} score {} nodes {} nps {} time {} pv {}\n",
                    depth, max_sel_depth, score_str, nodes_visited, nps, static_cast<int>(seconds * 1000),
                    (best_move_root != NO_MOVE ? Types::move_to_string(best_move_root) : "none"))
                    << std::flush;
            }
            return best_move_root;
        }
    };
}