#ifndef PEASANT_TT_HPP
#define PEASANT_TT_HPP

#include <vector>
#include "position.hpp"

// TODO: make this as space-efficient as possible, probably
struct TTEntry
{
    TTEntry()
      : depth(-1)
    {
    }

    TTEntry(const Position& _pos, int _alpha, int _beta, int _lower_bound, int _upper_bound, int _depth)
      : pos(_pos),
        alpha(_alpha),
        beta(_beta),
        lower_bound(_lower_bound),
        upper_bound(_upper_bound),
        depth(_depth)
    {
    }

    Position pos;
    int alpha;
    int beta;
    int lower_bound;
    int upper_bound;
    int depth;                                  // <0 means invalid entry
};


class TranspositionTable
{
public:
    TranspositionTable(std::size_t num_buckets, std::size_t num_slots_per_bucket);
    void insert(std::uint64_t hash, const TTEntry& entry);
    const TTEntry* fetch(std::uint64_t hash, const Position& pos) const;
    std::size_t get_bucket_index(std::uint64_t hash) const;

private:
    std::vector<TTEntry> m_entries;
    std::size_t m_num_slots_per_bucket;
};


void init_zobrist();
std::uint64_t calc_hash(const Position& pos);

#endif
