#pragma once

#include <array>

#include "Types.h"
#include "Position.h"
#include "MoveGen.h"

namespace Pick {
	constexpr int MAX_MOVES = 256;

	using MoveGen::MoveList;
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

    int score_move(const Position::Position& pos, Move move, Move tt_move){
        if (move == tt_move) {
            return 1'000'000;                  // TT move always searched first
        }

        // Promotions
        if (move.is_promo()) {
            if (move.promotion_type() == PieceType::QUEEN) {
                return move.is_capture() ? 950'000 : 900'000;
            }
            else {
                return move.is_capture() ? 250'000 : 200'000;
            }
        }

        // Captures (MVV-LVA)
        if (move.is_capture()) {
            const auto& mailbox = pos.get_mailbox();
            PieceType victim = move.is_en_passant() ? PieceType::PAWN : mailbox[move.to()].type();
            PieceType attacker = pos.get_mailbox()[move.from()].type();

            return 700'000 + MVV_LVA[static_cast<int>(victim)][static_cast<int>(attacker)];
        }

        // Quiet moves
        if (move.is_castling()) {
            return 50'000;
        }

        // All other quiet moves are equal
        return 0;
    }

	class Picker {
    public:
		Picker(Position::Position& pos, MoveList& list, Move tt_move) 
            :moves{list} {
			
			for (size_t idx{}; idx < moves.size(); ++idx) {
				auto& move = moves[idx];
                scores[idx] = score_move(pos, move, tt_move);
			}
		}

        Move next_legal(Position::Position& pos){
            Move move{};
            while ((move = next()) != NO_MOVE) {
                if (pos.is_legal(move)) {
                    return move;
                }
            }
            return NO_MOVE;
        }

    private:
        Move next() {
            if (current_index >= moves.size()) return NO_MOVE;

            int best_idx = current_index;
            for (size_t i = current_index + 1; i < moves.size(); ++i) {
                if (scores[i] > scores[best_idx]) {
                    best_idx = i;
                }
            }

            std::swap(moves[current_index], moves[best_idx]);
            std::swap(scores[current_index], scores[best_idx]);

            return moves[current_index++];
        }

	private:
		MoveList& moves;
		std::array<int, MAX_MOVES> scores{};
		size_t current_index{};
	};
}