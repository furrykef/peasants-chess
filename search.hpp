#ifndef PEASANT_SEARCH_HPP
#define PEASANT_SEARCH_HPP

struct SearchResult {
    int lower_bound;
    int upper_bound;
};

SearchResult search_root(unsigned int max_ply, std::uint64_t& num_searched_nodes);

#endif
