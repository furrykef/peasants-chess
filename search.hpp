#ifndef PEASANT_SEARCH_HPP
#define PEASANT_SEARCH_HPP

#include <boost/container/static_vector.hpp>
#include "bitboards.hpp"
#include "position.hpp"
#include "tt.hpp"

struct SearchResult {
    int lower_bound;
    int upper_bound;
    std::uint64_t num_leaves;
};

struct PerftMove
{
    Move move;
    std::uint64_t num_leaves;
};

const std::size_t MAX_DEPTH = 256;

typedef boost::container::static_vector<Move, MAX_DEPTH> Variation;

SearchResult search_node(int depth, const Position& pos, int alpha, int beta, TranspositionTable& tt, Variation& pv);
std::uint64_t perft_node(int depth, const Position& pos);
std::vector<PerftMove> split_perft_node(int depth, const Position& pos);

#endif
