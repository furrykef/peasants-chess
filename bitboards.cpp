#include "bitboards.hpp"

// From https://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating#Rotationby180degrees
// (stylistic tweaks have been made)
Bitboard rotate_bitboard(Bitboard x)
{
   const Bitboard h1 = 0x5555'5555'5555'5555ULL;
   const Bitboard h2 = 0x3333'3333'3333'3333ULL;
   const Bitboard h4 = 0x0f0f'0f0f'0f0f'0f0fULL;
   const Bitboard v1 = 0x00ff'00ff'00ff'00ffULL;
   const Bitboard v2 = 0x0000'ffff'0000'ffffULL;
   x = ((x >>  1) & h1) | ((x & h1) <<  1);
   x = ((x >>  2) & h2) | ((x & h2) <<  2);
   x = ((x >>  4) & h4) | ((x & h4) <<  4);
   x = ((x >>  8) & v1) | ((x & v1) <<  8);
   x = ((x >> 16) & v2) | ((x & v2) << 16);
   x = ( x >> 32)       | ( x       << 32);
   return x;
}
