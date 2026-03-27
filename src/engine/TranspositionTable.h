#pragma once
#include <vector>
#include <limits>

#include "Types.h"

namespace Constants {
    constexpr int MATE_SCORE = 32000;
    constexpr int INFINITE_SCORE = 32001;
    constexpr int MATE_THRESHOLD = 31000;
	constexpr int DRAW_SCORE = 0;
    constexpr std::int16_t SCORE_NONE = 32767;
}

namespace TranspositionTable {
	using namespace Types;

	using i16 = std::int16_t;
	using i8 = std::int8_t;



    enum NodeType : ui8 {
        EXACT = 0,
        LOWER_BOUND = 1, // Beta Cutoff (Fail High)
        UPPER_BOUND = 2  // Alpha (Fail Low)
    };

    struct alignas(32) TTEntry {
        ui64 key;          
        i16  score;        
        i16  static_eval;  
        Move move;         
        i8   depth;        
        ui8  type;         
        ui8  age;          
    };


    class TT {
    private:
        std::vector<TTEntry> table;
        size_t size_mask;
    public:
        TT(size_t sz_in_mb) {
            size_t entry_size = sizeof(TTEntry);
            size_t num_entries = (sz_in_mb * 1024 * 1024) / entry_size;

            // Next power of 2
            size_t p2_size = 1;
            while (p2_size * 2 <= num_entries) p2_size *= 2;

            table.resize(p2_size);
            size_mask = p2_size - 1;
        }

        // Mate score adjustment
        static i16 score_to_tt(int score, int ply) {
            if (score > Constants::MATE_THRESHOLD) return static_cast<i16>(score + ply);
            if (score < -Constants::MATE_THRESHOLD) return static_cast<i16>(score - ply);
            return static_cast<i16>(score);
        }

        static i16 score_from_tt(int score, int ply) {
            if (score > Constants::MATE_THRESHOLD) return static_cast<i16>(score - ply);
            if (score < -Constants::MATE_THRESHOLD) return static_cast<i16>(score + ply);
            return static_cast<i16>(score);
        }

        void store(ui64 key, int depth, int score, Move best_move, ui8 type, int ply, ui8 current_age, i16 eval) {
            size_t index = key & size_mask;

            TTEntry& e = table[index];

            bool modern = (e.age == current_age);
            bool replace = (e.key == 0) || (!modern) || (depth >= e.depth);

            if (replace) {
                e.key = key;
                e.score = score_to_tt(score, ply);
                e.move = best_move;
                e.depth = static_cast<i8>(depth);
                e.type = type;
                e.age = current_age;

                // Only update eval if the new one is valid
                if (eval != Constants::SCORE_NONE) {
                    e.static_eval = eval;
                }
            }
            // Even if we don't replace the search entry, we can "backfill" the eval
            else if (e.key == key && e.static_eval == Constants::SCORE_NONE) {
                e.static_eval = eval;
            }
        }

        void store_eval(ui64 key, i16 eval, ui8 age) {
            size_t index = key & size_mask;
            TTEntry& e = table[index];

            // Case 1: The entry is for this exact position
            if (e.key == key) {
                e.static_eval = eval;
                return;
            }

            // Case 2: The slot is empty or contains an "Old Age" entry
            // We can safely initialize it as a fresh Eval entry.
            if (e.key == 0 || e.age != age) {
                e.key = key;
                e.score = 0;
                e.static_eval = eval;
                e.move = NO_MOVE;
                e.depth = -1; // -1 indicates no search has happened here yet
                e.type = 0;
                e.age = age;
            }
        }

        // Probe (return full entry)
        TTEntry* probe(ui64 key){
            size_t index = key & size_mask;
            TTEntry& e = table[index];
            if (e.key == key) {
                return &e;
            }
            return nullptr;
        }

        void clear() {
            std::fill(table.begin(), table.end(), TTEntry{});
        }
    };
}