#ifndef PEASANT_POSITION_HPP
#define PEASANT_POSITION_HPP

#include <optional>
#include "bitboards.hpp"

struct Position
{
    Bitboard my_pawns;
    Bitboard their_pawns;
    std::optional<unsigned int> en_passant_bitnum;

    bool operator==(const Position& rhs) const {
        return my_pawns == rhs.my_pawns &&
               their_pawns == rhs.their_pawns &&
               en_passant_bitnum == rhs.en_passant_bitnum;
    }

    bool operator!=(const Position& rhs) const { return !(*this == rhs); }
};

struct Move
{
    unsigned int src_bitnum;
    unsigned int dest_bitnum;
};

#endif
