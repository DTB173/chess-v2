#pragma once

#include <chrono>
#include <array>
#include <iostream>
#include <format>
#include <atomic>

#include "Types.h"
#include "Position.h"
#include "MoveGen.h"
#include "TranspositionTable.h"
#include "See.h"
#include "Evaluationv2.h"
#include "MovePick.h"

namespace Search {

	namespace Search_Constants {
		constexpr int INF = 32'001;
		constexpr int MATE_VAL = 32'000;
		constexpr int MAX_PLY = 64;

		constexpr int MAX_HISTORY = 16'384;
		constexpr int HISTORY_GRAVITY = 256;
	}

	const std::array<std::array<int, 64>, 64> LMR_TABLE = []() {
		std::array<std::array<int, 64>, 64> table{};
		for (int depth = 1; depth < 64; ++depth) {
			for (int moves = 1; moves < 64; ++moves) {
				table[depth][moves] = static_cast<int>((std::log(depth) + std::log(moves)) / 2.25);
			}
		}
		return table;
	}();

	using namespace Types;
	using MoveGen::MoveList;

	using KillerArr = std::array<std::array<Move, Search_Constants::MAX_PLY>, 2>;
	using HistoryArr = std::array<std::array<std::array<int, Search_Constants::MAX_PLY>, Search_Constants::MAX_PLY>, 2>;
	using PvArr = std::array<std::array<Move, Search_Constants::MAX_PLY>, Search_Constants::MAX_PLY>;

	class Searcher {
	public:
		Move start_search(Position::Position& pos, size_t target_depth, TimeLimits limits);
		void clear();
		static inline std::atomic <bool> stop_signal{};
		static inline TranspositionTable::TT tt;
	private:
		int negamax(Position::Position& pos, int depth, int ply, int alpha, int beta, bool allowed_null);
		int quiescence(Position::Position& pos, int alpha, int beta, int ply);

		bool probe_tt(ui64 key, int depth, int ply, int alpha, int beta, int& tt_score, Move& tt_move);
		void store_tt(ui64 key, int depth, int ply, int score, int alpha_orig, int beta, Move best_move, int static_eval);
		void update_history(const Move move, Color side, int bonus);

		bool should_stop();

		template<bool is_loop>
		bool check_time() {
			if (time_up) return true;

			constexpr auto mask = is_loop ? 127 : 2047;
			if ((nodes_visited & mask) == 0)
				time_up = should_stop();

			return time_up;
		}

		inline double time_elapsed_ms(std::chrono::steady_clock::time_point start);

		void send_uci(int depth, int score) const;
		std::string pv_str()const;

	private:
		HistoryArr history{};
		KillerArr killers{};
		struct Pv_Line {
			PvArr table;
			std::array<int, Search_Constants::MAX_PLY> length;
		}pv;


		std::chrono::steady_clock::time_point search_start{};
		double max_time_ms{};
		bool time_up{};

		ui64 nodes_visited{};
		int max_sel_depth{};
		Move best_move_root{};
		ui8 current_age{};
	};

	inline bool Searcher::should_stop() {
		if (time_up) return true;

		if (stop_signal.load(std::memory_order_relaxed)) {
			return true;
		}

		double elapsed_ms = time_elapsed_ms(search_start);
		constexpr int overhead = 10;

		if (elapsed_ms >= (max_time_ms - overhead)) {
			return true;
		}

		return false;
	}

	inline double Searcher::time_elapsed_ms(std::chrono::steady_clock::time_point start)
	{
		auto now = std::chrono::steady_clock::now();
		return std::chrono::duration<double, std::milli>(now - start).count();
	}

	inline std::string Searcher::pv_str()const {
		std::string s;
		s.reserve(pv.length[0] * 5);
		for (int i{}; i < pv.length[0]; ++i) {
			s += move_to_string(pv.table[0][i]) + ' ';
		}
		return s;
	}
	inline void Searcher::send_uci(int depth, int score)const {
		std::string uci_output{};
		std::string pv_string{};

		// Calculate stats
		const auto current_time = std::chrono::steady_clock::now();
		const double elapsed_sec = std::chrono::duration<double>(current_time - search_start).count();
		const uint64_t nps = (elapsed_sec > 0.001)
			? static_cast<uint64_t>(nodes_visited / elapsed_sec)
			: 0;

		// Format score for UCI (mate / cp)
		std::string score_str;
		if (std::abs(score) >= Search_Constants::MATE_VAL - 100) {
			int dist = (Search_Constants::MATE_VAL - std::abs(score) + 1) / 2;
			score_str = std::string("mate ") + (score > 0 ? "" : "-") + std::to_string(dist);
		}
		else {
			score_str = "cp " + std::to_string(score);
		}

		// UCI info line
		pv_string = pv_str();
		uci_output = std::format(
			"info depth {} seldepth {} score {} nodes {} hashfull {} nps {} time {} pv {}",
			depth,
			max_sel_depth,
			score_str,
			nodes_visited,
			tt.get_hashfull(),
			nps,
			static_cast<int>(elapsed_sec * 1000),
			pv_string
		);

		std::cout << uci_output << std::endl;
	}

	inline bool Searcher::probe_tt(ui64 key, int depth, int ply, int alpha, int beta, int& tt_score, Move& tt_move) {
		TranspositionTable::TTEntry* entry = tt.probe(key);
		if (!entry) return false;

		tt_move = entry->move();
		tt_score = TranspositionTable::TT::score_from_tt(entry->score_, ply);

		if (entry->depth_ >= depth) {
			if (entry->type() == TranspositionTable::EXACT)                            return true;
			if (entry->type() == TranspositionTable::LOWER_BOUND && tt_score >= beta)  return true;
			if (entry->type() == TranspositionTable::UPPER_BOUND && tt_score <= alpha) return true;
		}
		return false;
	}

	inline void Searcher::store_tt(ui64 key, int depth, int ply, int score, int alpha_orig, int beta, Move best_move, int static_eval){
		ui8 type = TranspositionTable::UPPER_BOUND;
		if      (score >= beta)      type = TranspositionTable::LOWER_BOUND;
		else if (score > alpha_orig) type = TranspositionTable::EXACT;

		tt.store(key, depth, score, best_move, type, ply, current_age, static_eval);
	}

	inline void Searcher::update_history(Move move, Color side, int bonus) {
		constexpr int MAX = Search_Constants::MAX_HISTORY;
		int delta = std::clamp(bonus, -MAX, MAX);
		int& val = history[static_cast<int>(side) - 1][move.from()][move.to()];
		val += delta - val * std::abs(delta) / MAX;
	}

	inline int Searcher::quiescence(Position::Position& pos, int alpha, int beta, int ply) {
		++nodes_visited;

		if (ply > max_sel_depth) max_sel_depth = ply;

		Move tt_move{};
		int tt_score = 0;
		if (probe_tt(pos.get_zobrist_key(), 0, ply, alpha, beta, tt_score, tt_move)) return tt_score;

		// 1. ============ Standing Pat =============
		int static_eval = Eval::evaluate<false>(pos, tt, current_age);

		bool in_check = pos.is_in_check(pos.turn());

		if (!in_check) {
			// If our static evaluation is already enough to cause a beta cutoff, stop.
			if (static_eval >= beta) return beta;
			// If the static eval is better than alpha, update alpha.
			if (static_eval > alpha) alpha = static_eval;
		}

		if (ply >= Search_Constants::MAX_PLY) return static_eval;
		pv.length[ply] = ply;

		// 2. =========== Move Generation ===========
		MoveGen::MoveList moves;
		MoveGen::generate_captures(pos, moves);
		if (in_check) {
			MoveGen::generate_quiets(pos, moves);
		}
		else {
			MoveGen::generate_quiet_promotions(pos, moves);
		}
		Pick::Picker mp(pos, moves, tt_move, NO_MOVE, history);

		// 3. ========== Main Search ================
		Move move{};
		Move best_move{};
		while ((move = mp.next_legal(pos)) != NO_MOVE) {
			pos.make_move(move);
			int score = -quiescence(pos, -beta, -alpha, ply + 1);
			pos.undo_move();

			if (check_time<true>()) return 0;
			if (score >= beta) return beta;
			if (score > alpha) {
				alpha = score;
				best_move = move;
			}
		}

		return alpha;
	}

	inline int Searcher::negamax(Position::Position& pos, int depth, int ply, int alpha, int beta, bool allowed_null) {
		// 1. ========= Terminal checks =============
		// if its draw or we run out of time, we stop searching
		if (ply > 0 && pos.is_draw() || check_time<false>()) return 0;

		// 2. ============== TT Lookup ==============

		int alpha_orig = alpha;
		Move tt_move = NO_MOVE;
		int tt_score = 0;
		const bool pv_node = beta - alpha > 1;
		if (probe_tt(pos.get_zobrist_key(), depth, ply, alpha, beta, tt_score, tt_move) && ply > 0 && !pv_node)	return tt_score;

		// 3. ========== Quiescence at leafs ========
		if (depth <= 0) return quiescence(pos, alpha, beta, ply);

		nodes_visited++;

		Color us = pos.turn();
		bool in_check = pos.is_in_check(us);

		// safety cutoff
		int static_eval = Eval::evaluate<false>(pos, tt, current_age);
		if (ply >= Search_Constants::MAX_PLY) return static_eval;
		pv.length[ply] = ply;

		// 4. ========= Null Move Pruning ===========
		if (allowed_null && depth >= 3 && !in_check && ply > 0 && pos.has_non_pawn_material(pos.turn()) && static_eval >= beta) {
			int R = 2 + depth / 3;
			pos.make_null_move();
			int score = -negamax(pos, depth - 1 - R, ply + 1, -beta, -(beta - 1), false);
			pos.undo_null_move();
			if (abs(score) >= Search_Constants::MATE_VAL - 100) return beta;
			if (score >= beta) return score;
		}

		// 5. ============ Move generation ==========
		std::array<Move, 2> ply_killers = { killers[0][ply], killers[1][ply] };
		Move pv_move = (pv.length[ply] > ply) ? pv.table[ply][ply] : NO_MOVE;

		MoveList moves;
		MoveGen::generate_all_moves(pos, moves);
		Pick::Picker mp(pos, moves, tt_move, pv_move, history, ply_killers);

		// 6. ========== Main search setup ==========
		Move best_move{};
		int best_score{ -Search_Constants::INF };

		int move_count{};
		Move move{};

		MoveList quiets_tried;
		int n_quiets{};

		while ((move = mp.next_legal(pos)) != NO_MOVE) {
			move_count++;

			if (check_time<true>()) return 0;

			Color us = pos.turn();

			pos.make_move(move);

			// 6.1 ============ LMR =================
			int score{};
			if (depth >= 3 && move_count > 3 && !in_check && !move.is_tactical()) {
				int reduction = LMR_TABLE[std::min(depth,63)][std::min(move_count, 63)];

				int hist = history[static_cast<int>(us) - 1][move.from()][move.to()];

				// dont reduce killers and moves with good history
				if (move == ply_killers[0] || move == ply_killers[1]) --reduction;
				if (hist > Search_Constants::MAX_HISTORY/8) --reduction;

				// dont reduce too much
				reduction = std::clamp(reduction, 0, depth - 2);

				score = -negamax(pos, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, true);

				// fallback research
				if (score > alpha) {
					score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
				}
			}
			else {
				score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, true);
			}

			pos.undo_move();
			if (check_time<true>()) return 0;
			if (!move.is_capture() && n_quiets < 64) quiets_tried.push_back(move);

			// 6.2 ========== Beta cutoff ===========
			if (score > best_score) {
				best_score = score;
				best_move = move;
				if (ply == 0) best_move_root = move;
			}

			if (score > alpha) {
				alpha = score;

				// PV update
				pv.table[ply][ply] = move;
				for (int i = ply + 1; i < pv.length[ply + 1]; ++i) {
					pv.table[ply][i] = pv.table[ply + 1][i];
				}
				pv.length[ply] = pv.length[ply + 1];

				if (score >= beta) break;
			}
		}

		if (move_count == 0) {
			return in_check ? -Search_Constants::MATE_VAL + ply : 0;
		}

		// 7. ======== Update Heuristics on Cutoff ========
		if (best_score >= beta && !best_move.is_tactical()) {
			int bonus = std::min(depth * depth, 1600);
			update_history(best_move, pos.turn(), bonus);

			for (int i = 0; i < quiets_tried.size() - 1; i++) {
				update_history(quiets_tried[i], pos.turn(), -bonus);
			}

			if (best_move != killers[0][ply]) {
				killers[1][ply] = killers[0][ply];
				killers[0][ply] = best_move;
			}
		}
		
		// 8. =============== TT store ====================
		store_tt(pos.get_zobrist_key(), depth, ply, best_score, alpha_orig, beta, best_move, static_eval);

		return best_score;
	}

	inline Move Searcher::start_search(Position::Position& pos, size_t target_depth, TimeLimits limits) {
		best_move_root = NO_MOVE;
		nodes_visited = 0;
		max_sel_depth = 0;
		max_time_ms = limits.hard_ms;
		current_age++;
		time_up = false;
		search_start = std::chrono::steady_clock::now();

		Move best_so_far = NO_MOVE;
		int alpha = -Search_Constants::INF;
		int beta = +Search_Constants::INF;

		// fallback if we cant find a good move in given time
		MoveList moves;
		MoveGen::generate_all_moves(pos, moves);
		Pick::Picker initial_picker(pos, moves, NO_MOVE, NO_MOVE, history);
		best_move_root = initial_picker.next_legal(pos);

		for (int depth = 1; depth <= target_depth; ++depth) {

			// Early exit before starting expensive iteration
			if ((time_elapsed_ms(search_start) >= limits.soft_ms) && depth > 1) break;

			// Search start and updates
			int score = negamax(pos, depth, 0, alpha, beta, true);

			if (check_time<false>()) break;

			best_so_far = (best_move_root != NO_MOVE) ? best_move_root : NO_MOVE;

			send_uci(depth, score);
		}
		return best_so_far;
	}

	void Searcher::clear() {
		tt.clear();
		Eval::pawns_cache.clear();
		history = {};
		killers = {};
		pv = {};
	}
}