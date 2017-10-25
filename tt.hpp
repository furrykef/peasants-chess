#ifndef PEASANT_TT_HPP
#define PEASANT_TT_HPP

#include <vector>
#include "search.hpp"


// TODO: make this as space-efficient as possible, probably
struct TTEntry
{
    TTEntry()
      : depth(-1)
    {
    }

    Position pos;
    int lower_bound;
    int upper_bound;
    int depth;                                  // <0 means invalid entry
};


class TranspositionTable
{
public:
    TranspositionTable(std::size_t size);
    void insert(std::uint64_t hash, const TTEntry& entry);
    const TTEntry* fetch(std::uint64_t hash, const Position& pos) const;

private:
    std::vector<TTEntry> m_smart;               // prioritizes higher depth
    std::vector<TTEntry> m_forgetful;           // always-replace
};


void init_zobrist();
std::uint64_t calc_hash(const Position& pos);

#endif
