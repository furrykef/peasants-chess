#include <algorithm>
#include <boost/container/static_vector.hpp>
#include "search.hpp"
#include "bitboards.hpp"

const Bitboard WHITE_INITIAL_POS = 0x0000'0000'00ff'ff00ULL;
const Bitboard BLACK_INITIAL_POS = 0x00ff'ff00'0000'0000ULL;

// The maximum number of moves that can be made in a turn (an overestimate)
const int MAX_BRANCHES = 64;

const unsigned int NO_EN_PASSANT = UINT_MAX;    // note that 1 << NO_EN_PASSANT is 0

struct Position
{
    Bitboard my_pawns;
    Bitboard their_pawns;
    unsigned int en_passant_bitnum;             // NO_EN_PASSANT for, well, no en passant
};

struct Move
{
    Position new_pos;
    bool is_capture;
};

typedef boost::container::static_vector<Move, MAX_BRANCHES> MoveList;


namespace
{
SearchResult search_node(unsigned int depth,
    const Position& pos,
    int alpha,
    int beta,
    std::uint64_t& num_searched_nodes);
void gen_moves(MoveList& movelist, const Position& pos);
Position flip_board(const Position& pos);
SearchResult negate_search_result(SearchResult result);
}

// max_ply must be at least 1
// @TODO@ -- pass in lower and upper bounds
SearchResult search_root(unsigned int max_ply, std::uint64_t& num_searched_nodes)
{
    num_searched_nodes = 0;
    Position start_pos = {
        WHITE_INITIAL_POS,
        BLACK_INITIAL_POS,
        NO_EN_PASSANT
    };
    return search_node(max_ply, start_pos, -1, 1, num_searched_nodes);
}

namespace
{

SearchResult search_node(unsigned int depth,
    const Position& pos,
    int alpha,
    int beta,
    std::uint64_t& num_searched_nodes)
{
    ++num_searched_nodes;

    if (pos.their_pawns & 0x0000'0000'0000'ff00LL) {
        // An enemy pawn is on my second rank! I've lost!
        return {-1, -1};
    }

    // @TODO@ -- perhaps check for positions that will clearly be stalemate (all files dead)

    // @TODO@ -- perhaps check for passed pawns and don't stop searching if there are any
    if (depth == 0) {
        // Result is unknown
        return {alpha, 1};
    }

    MoveList movelist;
    gen_moves(movelist, pos);

    if (movelist.size() == 0) {
        // Stalemate
        // @TODO@ -- shouldn't get here once we've got better drawn position detection
        return {0, 0};
    }

    // @TODO@ -- sort moves

    int best_lower_bound = -1;
    int best_upper_bound = -1;
    for (const Move& move : movelist) {
        SearchResult child_result = search_node(depth - 1,
                                                flip_board(move.new_pos),
                                                -beta,
                                                -alpha,
                                                num_searched_nodes);
        child_result = negate_search_result(child_result);      // our score is opposite of opponent's score
        best_lower_bound = std::max(best_lower_bound, child_result.lower_bound);
        alpha = std::max(alpha, child_result.lower_bound);
        if (alpha >= beta) {
            // If we don't examine all nodes, we can't measure the upper bound
            // So we use 1 instead
            return {child_result.lower_bound, 1};
        }
        best_upper_bound = std::max(best_upper_bound, child_result.upper_bound);
    }

    return {best_lower_bound, best_upper_bound};
}

// This function only generates moves in the order they're found; no ordering is done.
void gen_moves(MoveList& movelist, const Position& pos)
{
    Bitboard all_pawns = pos.my_pawns | pos.their_pawns;
    Bitboard en_passant_bit = pos.en_passant_bitnum;

    for (unsigned int bitnum = 8; bitnum < 56; ++bitnum) {
        Bitboard bit = 1ULL << bitnum;
        if (pos.my_pawns & bit) {
            // We've found one of my pawns.
            // First, try to advance it.
            Bitboard advance_dest = bit << 8;
            if (!(all_pawns & advance_dest)) {
                // The space in front is empty; proceed
                Bitboard my_new_pawns = (pos.my_pawns | advance_dest) & ~bit;
                Move move = {{my_new_pawns, pos.their_pawns, NO_EN_PASSANT}, false};
                movelist.push_back(move);

                // Try two-square advance
                // @TODO@ -- code too similar to above
                bool on_second_rank = bitnum < 16;
                Bitboard long_advance_dest = bit << 16;
                if (on_second_rank && !(all_pawns & long_advance_dest)) {
                    Bitboard my_new_pawns = (pos.my_pawns | long_advance_dest) & ~bit;
                    Move move = {{my_new_pawns, pos.their_pawns, bitnum + 8}, false};
                    movelist.push_back(move);
                }
            }

            // 0 = searching for leftward capture
            // 1 = searching for rightward capture
            for (int i = 0; i < 2; ++i) {
                unsigned int column = bitnum % 8;       // 0 = rightmost column; 7 = leftmost

                // Don't test invalid captures (leftward capture on leftmost column, etc.)
                if ((i == 0 && column == 0) || (i == 1 && column == 7)) {
                    continue;
                }

                Bitboard dest = bit << ((i == 0) ? 9 : 7);
                if ((pos.their_pawns & dest) || dest == en_passant_bit) {
                    // Capture is possible
                    Bitboard my_new_pawns = (pos.my_pawns | dest) & ~bit;
                    Bitboard captured_pawn = (dest == en_passant_bit) ? (dest << 8) : dest;
                    Bitboard their_new_pawns = pos.their_pawns & ~captured_pawn;
                    Move move = {{my_new_pawns, their_new_pawns, NO_EN_PASSANT}, true};
                    movelist.push_back(move);
                }
            }
        }
    }
}

// Rotates the bitboards so that my_pawns and their_pawns are switched and rotated 180 degrees
Position flip_board(const Position& pos)
{
    return {rotate_bitboard(pos.their_pawns),
            rotate_bitboard(pos.my_pawns),
            64 - pos.en_passant_bitnum};
}

// Negates results for negamax
SearchResult negate_search_result(SearchResult result)
{
    return {-result.upper_bound, -result.lower_bound};
}

} // anon namespace
