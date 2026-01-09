module;
#include <vector>
#include <limits>
export module TranspositionTable;
import Types;

export namespace Constants {
    constexpr int MATE_SCORE = 32000;
    constexpr int INFINITE_SCORE = 32001;
    constexpr int MATE_THRESHOLD = 31000;
	constexpr int DRAW_SCORE = 0;
}

export namespace TranspositionTable {
	using namespace Types;

	using i16 = std::int16_t;
	using i8 = std::int8_t;



    enum NodeType : ui8 {
        EXACT = 0,
        LOWER_BOUND = 1, // Beta Cutoff (Fail High)
        UPPER_BOUND = 2  // Alpha (Fail Low)
    };

    struct TTEntry {
        ui64 zobrist_key; 
        i16 score;    
        Move move;    
        i8 depth;     
        ui8 type;         
    };

    class TranspositionTable {
    private:
        std::vector<TTEntry> table;
        size_t size_mask;
    public:
        TranspositionTable(size_t sz_in_mb) {
            size_t num_entries = (sz_in_mb * 1024 * 1024) / sizeof(TTEntry);

            size_t p2_size = 1;
            while (p2_size * 2 <= num_entries) p2_size *= 2;

            table.resize(p2_size, { 0 });
            size_mask = p2_size - 1;
        }

        // Helper to adjust mate scores relative to the root
        // This prevents the engine from getting confused by cached mates
        static int16_t score_to_tt(int score, int ply) {
            if (score > Constants::MATE_THRESHOLD) return (int16_t)(score + ply);
            if (score < -Constants::MATE_THRESHOLD) return (int16_t)(score - ply);
            return static_cast<i16>(score);
        }

        static int16_t score_from_tt(int score, int ply) {
            if (score > Constants::MATE_THRESHOLD) return (int16_t)(score - ply);
            if (score < -Constants::MATE_THRESHOLD) return (int16_t)(score + ply);
            return static_cast<i16>(score);
        }

        void store(ui64 key, int depth, int score, Move best_move, ui8 type, int ply) {
            size_t index = key & size_mask;

            // "Depth-Preferred" Replacement Strategy:
            // Only overwrite if the new search is deeper or it's a completely new position
            if (table[index].zobrist_key == 0 || depth >= table[index].depth) {
                table[index].zobrist_key = key;
                table[index].score = score_to_tt(score, ply);
                table[index].move = best_move;
                table[index].depth = (int8_t)depth;
                table[index].type = type;
            }
        }

        TTEntry* probe(ui64 key) {
            size_t index = key & size_mask;
            if (table[index].zobrist_key == key) {
                return &table[index];
            }
            return nullptr;
        }

        void clear() {
            std::fill(table.begin(), table.end(), TTEntry{ 0 });
        }
    };
}