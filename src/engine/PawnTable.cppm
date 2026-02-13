module;
#include <vector>

export module PawnTable;
import Types;
export namespace PawnTable {
    using namespace Types;

    struct alignas(32) PawnEntry {
        ui64 pawn_key;    
        i16  score_mg;    
        i16  score_eg;    
        ui64 passed_pawns;
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

        void store(ui64 key, i16 score_mg, i16 score_eg, ui64 passed_pawns) {
            size_t index = key & size_mask;
            table[index] = { key, score_mg, score_eg, passed_pawns };
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