#ifndef PEASANT_POSITION_HPP
#define PEASANT_POSITION_HPP

#include "bitboards.hpp"

const unsigned int NO_EN_PASSANT = UINT_MAX;    // do not try 1 << NO_EN_PASSANT! It's UB!

struct Position
{
    Bitboard my_pawns;
    Bitboard their_pawns;
    unsigned int en_passant_bitnum;             // NO_EN_PASSANT if en passant is impossible

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
