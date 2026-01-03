module;
#include <random>
export module Zobrist;
import Types;

namespace Zobrist {
    using namespace Types;

    ui64 get_random() {
        static std::mt19937_64 rng(123456);
        std::uniform_int_distribution<ui64> dist(0, std::numeric_limits<ui64>::max());
        return dist(rng);
    }

    // 12 pieces * 64 squares
    export inline ui64 pieces[12][64];

    // 16 combinations of castling rights (K Q k q)
    export inline ui64 castling[16];

    // 8 files where a pawn just double-pushed
    export inline ui64 en_passant[8];

    // XOR'd in if it is Black's turn
    export inline ui64 side;

    export void init() {
        for (int i = 0; i < 12; ++i)
            for (int j = 0; j < 64; ++j)
                pieces[i][j] = get_random();

        for (int i = 0; i < 16; ++i) castling[i] = get_random();
        for (int i = 0; i < 8; ++i)  en_passant[i] = get_random();

        side = get_random();
    }
}