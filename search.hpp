#ifndef PEASANT_SEARCH_HPP
#define PEASANT_SEARCH_HPP

struct SearchResult {
    int lower_bound;
    int upper_bound;
    std::uint64_t num_leaves;
};

SearchResult search_root(unsigned int max_ply);
std::uint64_t perft_root(unsigned int depth);

#endif
