#pragma once
#include <vector>
#include <span>
#include "Types.h"

#ifndef CACHE_STATS
#define CACHE_STATS
#endif

namespace PawnTable {
    using namespace Types;

    struct SafetyScore {
        i16 mg, eg;
    };
    struct alignas(32) PawnEntry {
        ui64 pawn_key;    
        ui64 passed_pawns;
        i16  score_mg;    
        i16  score_eg;    
        SafetyScore safety_w, safety_b;
    };


    class PawnTable {
        std::vector<PawnEntry> table;
        size_t size_mask;
        ui64 probes = 0, hits = 0;
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

        void store(ui64 key, i16 score_mg, i16 score_eg, ui64 passed_pawns, Score safety[2]) {
            size_t index = key & size_mask;
            SafetyScore w, b;
            w = SafetyScore(safety[0].mg, safety[0].eg);
            b = SafetyScore(safety[1].mg, safety[1].eg);
            table[index] = { key, passed_pawns, score_eg,score_mg, w, b};
        }

        // Probe (return full entry)
        PawnEntry* probe(ui64 key) {
#ifdef CACHE_STATS
            ++probes;
#endif
            size_t index = key & size_mask;
            PawnEntry& e = table[index];
            if (e.pawn_key == key) {
#ifdef CACHE_STATS
                ++hits;
#endif
                return &e;
            }
            return nullptr;
        }

        void print_stats() const {
            std::printf("pawn cache: %llu probes, %llu hits (%.1f%%)\n",
                (unsigned long long)probes, (unsigned long long)hits,
                probes ? 100.0 * hits / probes : 0.0);
        }

        void reset_stats() { probes = hits = 0; }

        void clear() {
            std::fill(table.begin(), table.end(), PawnEntry{});
        }
    };
}

#undef CACHE_STATS