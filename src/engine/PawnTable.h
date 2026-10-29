#pragma once
#include <vector>
#include <span>
#include "Types.h"

namespace PawnTable {
    using namespace Types;

    struct alignas(32) PawnEntry {
        ui64 pawn_key;    
        ui64 passed_pawns;
        i16  score_mg;    
        i16  score_eg;    
        i16  safety_w, safety_b;
        i16  shelter_w_mg, shelter_w_eg;
        i16  shelter_b_mg, shelter_b_eg;
    };


    class PawnTable {
        std::vector<PawnEntry> table;
        size_t size_mask;
    public:
        PawnTable(size_t sz_in_mb) {
            size_t entry_size = sizeof(PawnEntry);
            size_t num_entries = (sz_in_mb * 1024 * 1024) / entry_size;

            // Next power of 2
            size_t p2_size = 1;
            while (p2_size * 2 <= num_entries) p2_size *= 2;

            table.resize(p2_size);
            size_mask = p2_size - 1;
        }

        void store(ui64 key, i16 score_mg, i16 score_eg, ui64 passed_pawns, std::span<i16, 2> safety) {
            size_t index = key & size_mask;
            table[index] = { key, passed_pawns, score_eg,score_mg, safety[0], safety[1]};
        }

        // Probe (return full entry)
        PawnEntry* probe(ui64 key) {
            size_t index = key & size_mask;
            PawnEntry& e = table[index];
            if (e.pawn_key == key) {
                return &e;
            }
            return nullptr;
        }

        void clear() {
            std::fill(table.begin(), table.end(), PawnEntry{});
        }
    };
}