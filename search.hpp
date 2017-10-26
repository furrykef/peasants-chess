#ifndef PEASANT_SEARCH_HPP
#define PEASANT_SEARCH_HPP

#include "bitboards.hpp"
#include "position.hpp"
#include "tt.hpp"

struct SearchResult {
    int lower_bound;
    int upper_bound;
    std::uint64_t num_leaves;
};

SearchResult search_node(int depth, const Position& pos, int alpha, int beta, TranspositionTable& tt);
std::uint64_t perft_node(int depth, const Position& pos);

#endif
