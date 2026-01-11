module;
#include <array>
export module See;
import Position;
import Types;
import Bitboards;
import PSQTables;
import AttackTables;

export namespace See {
    using namespace Types;

    Square get_lva(const Position::Position& pos, Types::ui64 attackers, Color side) {
        ui64 subset = attackers & pos.get_bitboard(PieceType::PAWN, side);
        if (subset) return Bitwise::pop_lsb(subset);

        subset = attackers & pos.get_bitboard(PieceType::KNIGHT, side);
        if (subset) return Bitwise::pop_lsb(subset);

        subset = attackers & pos.get_bitboard(PieceType::BISHOP, side);
        if (subset) return Bitwise::pop_lsb(subset);

        subset = attackers & pos.get_bitboard(PieceType::ROOK, side);
        if (subset) return Bitwise::pop_lsb(subset);

        subset = attackers & pos.get_bitboard(PieceType::QUEEN, side);
        if (subset) return Bitwise::pop_lsb(subset);

        subset = attackers & pos.get_bitboard(PieceType::KING, side);
        if (subset) return Bitwise::pop_lsb(subset);

        return Square::SQ_NONE;
    }

    int see(const Position::Position& pos, Move m) {
        if (!m.is_capture()) return 0;

        // 1. Initial State
        std::array<int, 32> gains{};
        int depth = 0;

        Square from = m.from();
        Square to = m.to();

        // Get initial victim value
        Piece victim = pos.get_mailbox()[to];
        PieceType victim_pt = (m.is_en_passant()) ? PieceType::PAWN : victim.type();
        gains[0] = PSQTables::get_material_value(victim_pt);

        ui64 occupancy = pos.get_total_occupancy();
        Color side = pos.turn();
        Piece attacker = pos.get_mailbox()[from];

        // 2. Simulation Loop
        while (from != Square::SQ_NONE) {
            depth++;
            // Calculate gain for this step
            gains[depth] = PSQTables::get_material_value(attacker.type()) - gains[depth - 1];

            // Early Exit: If the current side can't possibly improve their situation
            if (std::max(-gains[depth - 1], gains[depth]) < 0) break;

            // Update board state
            occupancy ^= (1ULL << from);
            side = opponent_of(side);

            // Find the SMALLEST attacker (LVA)
            ui64 attackers = pos.get_attacks_to(to, occupancy, side);
            from = get_lva(pos, attackers, side);

            if (from != Square::SQ_NONE) {
                attacker = pos.get_mailbox()[from];
                // If the King captures, the sequence ends (Kings can't be taken)
                if (attacker.type() == PieceType::KING) {
                    // Check if the opponent still has an attacker; if so, King can't capture
                    if (pos.get_attacks_to(to, occupancy, opponent_of(side))) {
                        // King is "attacked" on this square, so this capture is illegal/suicidal
                        from = Square::SQ_NONE;
                    }
                    else {
                        depth++;
                        gains[depth] = PSQTables::get_material_value(PieceType::KING) - gains[depth - 1];
                    }
                    break;
                }
            }
        }

        // 3. Back-propagation (Minimax)
        while (--depth > 0) {
            gains[depth - 1] = -std::max(-gains[depth - 1], gains[depth]);
        }

        return gains[0];
    }
}
