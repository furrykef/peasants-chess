#include <random>
#include <cassert>
#include "tt.hpp"

namespace
{
std::uint64_t g_zobrist_codes[8][0x10000];
std::uint64_t g_zobrist_en_passant[64];
}


TranspositionTable::TranspositionTable(std::size_t num_buckets, std::size_t num_slots_per_bucket)
  : m_entries(num_buckets*num_slots_per_bucket),
    m_num_slots_per_bucket(num_slots_per_bucket)
{
}

void TranspositionTable::insert(std::uint64_t hash, const TTEntry& entry)
{
    assert(hash == calc_hash(entry.pos));
    if (m_entries.size() == 0) {
        return;
    }
    std::uint64_t index = get_bucket_index(hash);
    std::size_t last_index = index + m_num_slots_per_bucket - 1;
    for (; index <= last_index; ++index) {
        if (entry.depth > m_entries[index].depth || index == last_index) {
            m_entries[index] = entry;
            return;
        } 
    }
}

const TTEntry* TranspositionTable::fetch(std::uint64_t hash, const Position& pos) const
{
    assert(hash == calc_hash(pos));
    if (m_entries.size() == 0) {
        return nullptr;
    }
    // @TODO@ -- duplicate code
    std::size_t index = get_bucket_index(hash);
    for (std::size_t i = 0; i < m_num_slots_per_bucket; ++i) {
        if (m_entries[index].depth > 0 && m_entries[index].pos == pos) {
            return &m_entries[index];
        }
    }
    return nullptr;
}


std::size_t TranspositionTable::get_bucket_index(std::uint64_t hash) const
{
    return hash % (m_entries.size()/m_num_slots_per_bucket) * m_num_slots_per_bucket;
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
