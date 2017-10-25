#ifndef PEASANT_SEARCH_HPP
#define PEASANT_SEARCH_HPP

#include "bitboards.hpp"

const unsigned int NO_EN_PASSANT = UINT_MAX;    // note that 1 << NO_EN_PASSANT is 0

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

struct SearchResult {
    int lower_bound;
    int upper_bound;
    std::uint64_t num_leaves;
};

SearchResult search_node(unsigned int depth, const Position& pos, int alpha, int beta);
std::uint64_t perft_node(unsigned int depth, const Position& pos);

#endif
