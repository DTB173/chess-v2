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

export namespace MoveSorter {
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
        // Pass tt_move here
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

        Types::Move next() {
            if (current_idx >= list.size()) return Move{};

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
	TranspositionTable::TranspositionTable tt(64); // 64 MB TT
    class Searcher {
    private:
        int max_depth{};
        Types::Move best_move_root{};
		int best_score_root{};
        ui64 nodes_visited{};

        int quiescence(Position::Position& pos, int alpha, int beta) {
            if (pos.is_draw()) {
                return 0;
            }

            // 1. Check handling
			Color us = pos.get_metadata().side_to_move();
            bool in_check = pos.is_in_check(us);

            // 2. Standing Pat (Only if not in check)
            if (!in_check) {
                int stand_pat = pos.evaluate();
                if (stand_pat >= beta) return beta;
                if (alpha < stand_pat) alpha = stand_pat;
            }

            // 3. Generate Moves
            // If in check, we need moves that escape check (often all legal moves)
            // If not in check, only captures
            MoveGen::MoveList list = in_check
                ? MoveGen::generate_all_moves(pos)
                : MoveGen::generate_captures(pos);

            // If in check and no moves, it's checkmate (handled by the loop returning alpha)
            // but if not in check and no captures, standing pat (alpha) is returned.

            MoveSorter::MovePicker picker(list, pos);
            Move move;

			int legal_moves = 0;
            while ((move = picker.next()) != Move{}) {
                pos.make_move(move);

                if (pos.is_in_check(us)) {
                    pos.undo_move();
                    continue;
                }
				++legal_moves;
                int score = -quiescence(pos, -beta, -alpha);
                pos.undo_move();

                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }

            if (in_check && legal_moves == 0) return -30000;
            return alpha;
        }

        int negamax(Position::Position& pos, int depth, int ply, int alpha, int beta, bool allowed_null) {
			nodes_visited++;

            if (pos.is_draw()) {
                return 0;  // Draw score
            }

            int alpha_orig = alpha;
            using namespace TranspositionTable;

            // 1. TT Probe
            TTEntry* entry = tt.probe(pos.get_zobrist_key());
            if (entry != nullptr && entry->depth >= depth) {
                int tt_score = tt.score_from_tt(entry->score, ply);
                if (entry->type == EXACT) return tt_score;
                else if (entry->type == LOWER_BOUND) alpha = std::max(alpha, tt_score);
                else if (entry->type == UPPER_BOUND) beta = std::min(beta, tt_score);
                if (alpha >= beta) return tt_score;
            }

            if (depth <= 0) return quiescence(pos, alpha, beta);

            int static_eval = pos.evaluate();

            if (depth <= 3 && static_eval - (150 * depth) >= beta) return beta;

            Color us = pos.get_metadata().side_to_move();

            bool has_material = (us == Color::WHITE) ?
                pos.has_non_pawn_material<Color::WHITE>() :
                pos.has_non_pawn_material<Color::BLACK>();

            if (allowed_null && depth >= 3 && !pos.is_in_check(us) && ply > 0 && has_material && static_eval >= beta - 50) {
                pos.make_null_move();
                // Pass 'false' to the next depth so it can't null move again
                int R = 3 + (depth / 6);
                int score = -negamax(pos, depth - 1 - R, ply + 1, -beta, -beta + 1, false);
                pos.undo_null_move();

                if (score >= beta) return beta;
            }

            // 2. Identify the TT move for sorting
            Move tt_move = (entry != nullptr) ? entry->move : Move{};

            MoveGen::MoveList list = MoveGen::generate_all_moves(pos);
            

            // 3. Pass tt_move to your picker so it searches it FIRST
            MoveSorter::MovePicker picker(list, pos, tt_move);

            Types::Move move;
            int legal_moves = 0;
            Move best_move_at_node = Move{}; // Local best move
            int best_score = -MATE_SCORE;    // Local best score
			int moves_searched = 0;

            while ((move = picker.next()) != Types::Move{}) {
                pos.make_move(move);
                if (pos.is_in_check(us)) {
                    pos.undo_move();
                    continue;
                }

                if (legal_moves == 0) {
                    best_move_at_node = move;
                    if (ply == 0) best_move_root = move;
                }
				moves_searched++;
                legal_moves++;
                int score;

				bool is_check = pos.is_in_check(opponent_of(us));
                // Principal Variation Search (PVS) + LMR
                if (moves_searched == 1) {
                    // Full window search for the first (presumably best) move
                    score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                }
                else {
                    // LMR Logic
                    if (depth >= 3 && moves_searched > 4 && !move.is_capture() && !pos.is_in_check(opponent_of(us))) {
                        score = -negamax(pos, depth - 2, ply + 1, -alpha - 1, -alpha, true);
                    }
                    else {
                        score = alpha + 1; // Force a full search if LMR isn't applicable
                    }

                    // Zero Window Search (PVS)
                    if (score > alpha) {
                        score = -negamax(pos, depth - 1, ply + 1, -alpha - 1, -alpha, true);
                        // If it still beats alpha, do a full window search
                        if (score > alpha && score < beta) {
                            score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
                        }
                    }
                }
                pos.undo_move();

                if (score > best_score) {
                    best_score = score;
                    best_move_at_node = move;
                }

                if (score > alpha) {
                    alpha = score;
                    if (ply == 0) best_move_root = move;
                }

                if (alpha >= beta) break; // Fail-high (LOWER_BOUND)
            }

            // 4. Correct Mate/Stalemate Detection
            if (legal_moves == 0) {
                if (pos.is_in_check(us)) return -MATE_SCORE + ply;
                return 0;
            }

            // 5. Store in TT using LOCAL best_score and best_move_at_node
            ui8 type = (best_score <= alpha_orig) ? UPPER_BOUND :
                (best_score >= beta) ? LOWER_BOUND : EXACT;

            if(best_move_at_node != Move{})
                tt.store(pos.get_zobrist_key(), depth, best_score, best_move_at_node, type, ply);

            if(ply == 0)
				best_move_root = best_move_at_node;
            return best_score;
        }
    public:
        Move start_search(Position::Position& pos, int target_depth) {
            best_move_root = Types::Move{}; // Reset
            Move final_move{};
			auto start_time = std::chrono::high_resolution_clock::now();
            // Iterative Deepening
			nodes_visited = 0;
            for (int d = 1; d <= target_depth; ++d) {
                // We search with a very wide window (-infinity to +infinity)
                int score = negamax(pos, d, 0, -30001, 30001, true);

                best_score_root = score;

                if (best_move_root != Move{})
                    final_move = best_move_root;

                // Output to see what's happening
                //std::cout << "Depth " << d
                //    << " | Score: " << score
                //    << " | Best Move: " << Types::move_to_string(best_move_root)
                //    << '\n';
            }

			auto end_time = std::chrono::high_resolution_clock::now();

            auto duration = end_time - start_time;
            double seconds = std::chrono::duration<double>(duration).count();
            uint64_t nps = (seconds > 0.001) ? static_cast<uint64_t>(nodes_visited / seconds) : 0;

            std::cout << std::format("[ Info ] depth {} nodes {} nps {} search time {:.3f} seconds\n", target_depth, nodes_visited, nps, seconds);
            std::cout << std::format("[ Res  ] Best Move {} with score {}\n", Types::move_to_string(best_move_root), best_score_root);
            return final_move;
        }
    };

}