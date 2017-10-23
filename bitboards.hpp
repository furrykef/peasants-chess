#ifndef PEASANT_BITBOARDS_HPP
#define PEASANT_BITBOARDS_HPP

// The most significant bit of a bitboard is the top-left square of the board.
// If the bitboard is oriented from white's POV, the most significant bit is a8.

#include <cstdint>

typedef std::uint64_t Bitboard;

Bitboard vflip_bitboard(Bitboard board);
Bitboard rotate_bitboard(Bitboard bitboard);

#endif
