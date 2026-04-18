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

    struct alignas(16) TTEntry {
        uint64_t key_{};          // 8B
        int16_t  score_{};        // 2B
        int16_t  static_eval_{};  // 2B
        Move     move_{};         // 2B
        int8_t   depth_{};        // 1B
        uint8_t  info_{};         // 1B (age << 2 | type)

        TTEntry() = default;

        TTEntry(uint64_t key, int16_t score, int16_t static_eval, Move m, int8_t depth, uint8_t type, uint8_t age)
            : key_(key), score_(score), static_eval_(static_eval), move_(m), depth_(depth) {
            info_ = (static_cast<uint8_t>(age) << 2) | (type & 0x3);
        }

        // Accessors
        uint8_t  age()  const { return info_ >> 2; }
        NodeType type() const { return static_cast<NodeType>(info_ & 0x3); }
        Move     move() const { return move_; }
    };

    static_assert(sizeof(TTEntry) == 16, "TTEntry must be 16 bytes!");

    struct alignas(64) Cluster {
        std::array<TTEntry, 4> data;
    };

    class TT {
    private:
        std::vector<Cluster> table;
        size_t size_mask;
    public:
        TT(size_t sz_in_mb) {
            size_t cluster_size = sizeof(Cluster); // 64 Bytes
            size_t num_clusters = (sz_in_mb * 1024 * 1024) / cluster_size;

            // Next power of 2 for the mask
            size_t p2_size = 1;
            while (p2_size * 2 <= num_clusters) p2_size *= 2;

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

        inline int replacement_score(const TTEntry& e, ui8 current_age) {
            if (e.key_ == 0) return -1000000;           // empty is best
            if (e.age() != current_age) return -50000;  // different generation is good to replace

            // Depth-based, but much less protective of old deep entries
            return e.depth_ + (e.type() == EXACT ? 6 : 0);
        }

        void store(ui64 key, int depth, int score, Move best_move,
            ui8 type, int ply, ui8 age, i16 eval)
        {
            Cluster& cluster = table[key & size_mask];
            i16 tt_score = score_to_tt(score, ply);

            TTEntry* best = nullptr;
            int worst_score = 1000000;

            for (auto& e : cluster.data) {

                // Exact match
                if (e.key_ == key) {

                    // Only overwrite if better
                    if (depth >= e.depth_) {
                        e = TTEntry(key, tt_score, eval, best_move, (i8)depth, type, age);
                    }
                    else {
                        if (best_move != NO_MOVE) e.move_ = best_move;
                        if (eval != Constants::SCORE_NONE) e.static_eval_ = eval;
                    }
                    return;
                }

                // Track worst entry for replacement
                int s = replacement_score(e, age);
                if (s < worst_score) {
                    worst_score = s;
                    best = &e;
                }
            }

            // Replace worst entry
            *best = TTEntry(key, tt_score, eval, best_move, (i8)depth, type, age);
        }

        void store_eval(ui64 key, i16 eval, ui8 age) {
            Cluster& cluster = table[key & size_mask];

            TTEntry* best = nullptr;
            int worst_score = 1000000;

            for (auto& e : cluster.data) {

                // Exact match ? just update
                if (e.key_ == key) {
                    e.static_eval_ = eval;
                    return;
                }

                int s = replacement_score(e, age);
                if (s < worst_score) {
                    worst_score = s;
                    best = &e;
                }
            }

            // Only replace if it's actually a weak slot
            if (best->key_ == 0 || best->age() != age) {
                *best = TTEntry(key, 0, eval, NO_MOVE, -1, 0, age);
            }
        }

        TTEntry* probe(ui64 key) {
            Cluster& cluster = table[key & size_mask];

            for (int i = 0; i < 4; ++i) {
                if (cluster.data[i].key_ == key) {
                    return &cluster.data[i];
                }
            }
            return nullptr;
        }

        int get_hashfull() const {
            int occupied = 0;
            size_t sample_clusters = std::min<size_t>(250, table.size());

            for (size_t i = 0; i < sample_clusters; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (table[i].data[j].key_ != 0) occupied++;
                }
            }
            return (int)((occupied * 1000) / (sample_clusters * 4));
        }

        void clear() {
            std::fill(table.begin(), table.end(), Cluster{});
        }
    };
}