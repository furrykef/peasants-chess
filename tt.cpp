#include <random>
#include <cassert>
#include "tt.hpp"

namespace
{
std::uint64_t g_zobrist_codes[8][0x10000];
std::uint64_t g_zobrist_en_passant[64];
}


TranspositionTable::TranspositionTable(std::size_t size)
  : m_smart(size/2),
    m_forgetful(size/2)
{
}

void TranspositionTable::insert(std::uint64_t hash, const TTEntry& entry)
{
    assert(hash == calc_hash(entry.pos));
    if (m_smart.size() == 0) {
        return;
    }
    std::uint64_t index = hash % m_smart.size();
    if (m_smart.size() > 0 && entry.depth > m_smart[index].depth) {
        m_smart[index] = entry;
    } else if(m_forgetful.size() > 0) {
        m_forgetful[index] = entry;
    }
}

const TTEntry* TranspositionTable::fetch(std::uint64_t hash, const Position& pos) const
{
    assert(hash == calc_hash(pos));
    if (m_smart.size() == 0) {
        return nullptr;
    }
    std::uint64_t index = hash % m_smart.size();
    if (m_smart.size() > 0 && m_smart[index].depth > 0 && m_smart[index].pos == pos) {
        return &m_smart[index];
    }
    if (m_forgetful.size() > 0 && m_forgetful[index].depth > 0 && m_forgetful[index].pos == pos) {
        return &m_forgetful[index];
    }
    return nullptr;
}


// Call this at program start
void init_zobrist()
{
    std::mt19937_64 rng;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 0x10000; ++j) {
            g_zobrist_codes[i][j] = rng();
        }
    }
    for (int i = 0; i < 64; ++i) {
        g_zobrist_en_passant[i] = rng();
    }
}


std::uint64_t calc_hash(const Position& pos)
{
    Bitboard my_pawns = pos.my_pawns;
    Bitboard their_pawns = pos.their_pawns;
    std::uint64_t hash = (pos.en_passant_bitnum != NO_EN_PASSANT) ? g_zobrist_en_passant[pos.en_passant_bitnum] : 0;
    hash ^= g_zobrist_codes[0][0xffff & my_pawns];
    hash ^= g_zobrist_codes[1][0xffff & (my_pawns >> 16)];
    hash ^= g_zobrist_codes[2][0xffff & (my_pawns >> 32)];
    hash ^= g_zobrist_codes[3][0xffff & (my_pawns >> 48)];
    hash ^= g_zobrist_codes[4][0xffff & their_pawns];
    hash ^= g_zobrist_codes[5][0xffff & (their_pawns >> 16)];
    hash ^= g_zobrist_codes[6][0xffff & (their_pawns >> 32)];
    hash ^= g_zobrist_codes[7][0xffff & (their_pawns >> 48)];
    return hash;
}
