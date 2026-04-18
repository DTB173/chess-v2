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
		constexpr int INF      = 32001;
		constexpr int MATE_VAL = 32000;
		constexpr int MAX_PLY  = 64;

		constexpr int TT_SIZE_MB = 256;
	}

	using namespace Types;
	using MoveGen::MoveList;

	TranspositionTable::TT tt(Search_Constants::TT_SIZE_MB);

	class Searcher {
	public:
		Move start_search(Position::Position& pos, int target_depth = 64, ui64 target_time_ms = 30 * 60 * 1000);
		static inline std::atomic <bool> stop_signal{};
	private:
		int negamax(Position::Position& pos, int depth, int ply, int alpha, int beta);
		int quiescence(Position::Position& pos, int alpha, int beta, int ply);
		bool probe_tt(ui64 key, int depth, int ply, int alpha, int beta, int& tt_score, Move& tt_move);
		void store_tt(ui64 key, int depth, int ply, int score, int alpha_orig, int beta, Move best_move, int static_eval);

		bool should_stop();
		void check_time() { time_up = (nodes_visited & 2047) == 0 && should_stop();	}
		bool check_time_loop() { time_up = (nodes_visited & 127) == 0 && should_stop(); return time_up; }
		inline double time_elapsed_ms(std::chrono::steady_clock::time_point start);

		void send_uci(int depth, int score, Move best_so_far) const;

	private:
		std::chrono::steady_clock::time_point search_start{};
		double max_time_ms{};
		bool time_up{};

		int nodes_visited{};
		int max_sel_depth{};
		Move best_move_root{};
		ui8 current_age{};
	};

	inline bool Searcher::should_stop() {
		if (time_up) return true;

		if (stop_signal.load(std::memory_order_relaxed)) {
			time_up = true;
			return true;
		}

		double elapsed_ms = time_elapsed_ms(search_start);
		constexpr int overhead = 10;

		if (elapsed_ms >= (max_time_ms - overhead)) {
			time_up = true;
			return true;
		}

		return false;
	}

	inline double Searcher::time_elapsed_ms(std::chrono::steady_clock::time_point start)
	{
		auto now = std::chrono::steady_clock::now();
		return std::chrono::duration<double, std::milli>(now - start).count();
	}

	inline void Searcher::send_uci(int depth, int score, Move best_so_far)const {
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
		pv_string = best_so_far != NO_MOVE ? move_to_string(best_so_far) : "";
		uci_output = std::format(
			"info depth {} seldepth {} score {} nodes {} hashfull {} nps {} time {} current_best {}",
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

	inline void Searcher::store_tt(ui64 key, int depth, int ply, int score, int alpha_orig, int beta, Move best_move, int static_eval) {
		ui8 type = TranspositionTable::UPPER_BOUND;
		if (score >= beta)           type = TranspositionTable::LOWER_BOUND;
		else if (score > alpha_orig) type = TranspositionTable::EXACT;

		tt.store(key, depth, score, best_move, type, ply, current_age, static_eval);
	}

	inline int Searcher::quiescence(Position::Position& pos, int alpha, int beta, int ply) {
		nodes_visited++;
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

		// 2. =========== Move Generation ===========
		MoveGen::MoveList moves;
		MoveGen::generate_captures_and_promos(pos, moves);
		Pick::Picker mp(pos, moves, tt_move);

		// 3. ========== Main Search ================
		Move move{};
		Move best_move{};
		while ((move = mp.next_legal(pos)) != NO_MOVE) {
			pos.make_move(move);
			int score = -quiescence(pos, -beta, -alpha, ply + 1);
			pos.undo_move();

			if (score >= beta) return beta;
			if (score > alpha) {
				alpha = score;
				best_move = move;
			}
		}

		return alpha;
	}

	inline int Searcher::negamax(Position::Position& pos, int depth, int ply, int alpha, int beta) {
		nodes_visited++;

		// 1. ========= Terminal checks =============

		// if its draw, we stop searching
		if (ply > 0 && pos.is_draw()) return 0;

		// periodical time check
		check_time();
		if (time_up) return 0;
		
		// 2. ============== TT Lookup ==============
		
		int alpha_orig = alpha;
		Move tt_move = NO_MOVE;
		int tt_score = 0;
		if (probe_tt(pos.get_zobrist_key(), depth, ply, alpha, beta, tt_score, tt_move)) {
			if(ply > 0) return tt_score;
		}
		
		// 3. ========== Quiescence at leafs ========
		if (depth <= 0) return quiescence(pos, alpha, beta, ply);

		Color us = pos.turn();
		bool in_check = pos.is_in_check(us);

		// safety cutoff
		int static_eval = Eval::evaluate<false>(pos, tt, current_age);
		if (ply >= 64) return static_eval;

		// 4. ============ Move generation ==========
		MoveList moves;
		MoveGen::generate_all_moves(pos, moves);
		Pick::Picker mp(pos, moves, tt_move);

		// 5. ========== Main search setup ==========
		Move best_move{};
		int best_score{-Search_Constants::INF};

		int legal_count{};
		Move move{};

		while ((move = mp.next_legal(pos)) != NO_MOVE) {
			legal_count++;

			if (check_time_loop()) return 0;

			pos.make_move(move);

			int score{};
			score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha);

			pos.undo_move();

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

		if (legal_count == 0) {
			return in_check ? -Search_Constants::MATE_VAL + ply : 0;
		}

		store_tt(pos.get_zobrist_key(), depth, ply, best_score, alpha_orig, beta, best_move, static_eval);
		return best_score;
	}

	inline Move Searcher::start_search(Position::Position& pos, int target_depth, ui64 target_time_ms) {
		best_move_root = NO_MOVE;
		nodes_visited = 0;
		max_sel_depth = 0;
		max_time_ms = target_time_ms;
		current_age++;
		time_up = false;

		search_start = std::chrono::steady_clock::now();
		const double soft_limit_ms = max_time_ms * 0.85;

		Move best_so_far = NO_MOVE;
		int alpha = -Search_Constants::INF;
		int beta = +Search_Constants::INF;

		for (int depth = 1; depth <= target_depth; ++depth) {

			// Early exit before starting expensive iteration
			if ((time_elapsed_ms(search_start) >= soft_limit_ms) && depth > 1) break;

			// Search start and updates
			int score = negamax(pos, depth, 0, alpha, beta);

			if (should_stop()) break;

			best_so_far = (best_move_root != NO_MOVE) ? best_move_root : NO_MOVE;

			send_uci(depth, score, best_so_far);
		}
		return best_so_far;
	}
}