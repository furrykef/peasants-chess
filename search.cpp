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


const Position START_POS = {
    WHITE_INITIAL_POS,
    BLACK_INITIAL_POS,
    NO_EN_PASSANT
};


namespace
{
SearchResult search_node(unsigned int depth,
    const Position& pos,
    int alpha,
    int beta,
    std::uint64_t& num_searched_nodes);
void gen_moves(MoveList& movelist, const Position& pos);
bool try_advance(MoveList& movelist, const Position& pos, Bitboard bit, unsigned int num_squares, unsigned int en_passant_bitnum);
void try_capture(MoveList& movelist, const Position& pos, Bitboard bit, int direction, Bitboard en_passant_bit);
Position flip_board(const Position& pos);
SearchResult negate_search_result(SearchResult result);
std::uint64_t perft_node(unsigned int depth, const Position& pos);
}

// max_ply must be at least 1
// @TODO@ -- pass in lower and upper bounds
SearchResult search_root(unsigned int max_ply, std::uint64_t& num_searched_nodes)
{
    num_searched_nodes = 0;
    return search_node(max_ply, START_POS, -1, 1, num_searched_nodes);
}


std::uint64_t perft_root(unsigned int depth)
{
    return perft_node(depth, START_POS);
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

    if (!pos.my_pawns || pos.their_pawns & 0x0000'0000'0000'ff00LL) {
        // I have no pawns or an enemy pawn is on my second rank! I've lost!
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
            return {alpha, 1};
        }
        best_upper_bound = std::max(best_upper_bound, child_result.upper_bound);
    }

    return {best_lower_bound, best_upper_bound};
}

// This function only generates moves in the order they're found; no ordering is done.
void gen_moves(MoveList& movelist, const Position& pos)
{
    for (unsigned int bitnum = 8; bitnum < 56; ++bitnum) {
        Bitboard bit = 1ULL << bitnum;
        if (pos.my_pawns & bit) {
            // We've found one of my pawns.
            // Try a one-square advance
            bool can_advance = try_advance(movelist, pos, bit, 1, NO_EN_PASSANT);

            // If successful, try a two-square advance if on second rank
            if (can_advance && bitnum < 16) {
                try_advance(movelist, pos, bit, 2, bitnum+8);
            }

            Bitboard en_passant_bit = 1ULL << pos.en_passant_bitnum;
            unsigned int column = bitnum % 8;       // 0 = rightmost column; 7 = leftmost
            // Don't test invalid captures (leftward capture on leftmost column, etc.)
            if (column != 7) {
                try_capture(movelist, pos, bit, -1, en_passant_bit);
            }
            if (column != 0) {
                try_capture(movelist, pos, bit, 1, en_passant_bit);
            }
        }
    }
}

// Only checks if the destination is occupied; two-square advances do not check if a pawn is in the way!
// (This should be done by only calling after checking the result of a one-square advance)
// Returns true if the square was unoccupied
bool try_advance(MoveList& movelist,
                 const Position& pos,
                 Bitboard bit,
                 unsigned int num_squares,
                 unsigned int new_en_passant_bitnum)
{
    Bitboard all_pawns = pos.my_pawns | pos.their_pawns;
    Bitboard dest = bit << (8*num_squares);
    if (!(all_pawns & dest)) {
        // The destination is empty; we can advance
        Bitboard my_new_pawns = (pos.my_pawns | dest) & ~bit;
        Move move = {{my_new_pawns, pos.their_pawns, new_en_passant_bitnum}, false};
        movelist.push_back(move);
        return true;
    }
    return false;
}

void try_capture(MoveList& movelist,
                 const Position& pos,
                 Bitboard bit,
                 int direction,                     // -1 = leftward; 1 = rightward
                 Bitboard en_passant_bit)
{
    Bitboard dest = bit << (8 - direction);
    if ((pos.their_pawns & dest) || dest == en_passant_bit) {
        // Capture is possible
        Bitboard my_new_pawns = (pos.my_pawns | dest) & ~bit;
        Bitboard captured_pawn = (dest == en_passant_bit) ? dest >> 8 : dest;
        Bitboard their_new_pawns = pos.their_pawns & ~captured_pawn;
        Move move = {{my_new_pawns, their_new_pawns, NO_EN_PASSANT}, true};
        movelist.push_back(move);
    }
}


std::uint64_t perft_node(unsigned int depth, const Position& pos)
{
    if (depth == 0) {
        return 1;
    }

    // @TODO@ -- copy/pasted from search_root
    if (!pos.my_pawns || pos.their_pawns & 0x0000'0000'0000'ff00LL) {
        // I have no pawns or an enemy pawn is on my second rank! I've lost!
        return 1;
    }

    MoveList movelist;
    gen_moves(movelist, pos);

    if (movelist.size() == 0) {
        // Stalemate
        return 1;
    }

    std::uint64_t leaves = 0;
    for (const Move& move : movelist) {
        leaves += perft_node(depth - 1, flip_board(move.new_pos));
    }

    return leaves;
}


// Rotates the bitboards so that my_pawns and their_pawns are switched and rotated 180 degrees
Position flip_board(const Position& pos)
{
    unsigned int en_passant = (pos.en_passant_bitnum != NO_EN_PASSANT) ? 63 - pos.en_passant_bitnum : NO_EN_PASSANT;
    return {rotate_bitboard(pos.their_pawns),
            rotate_bitboard(pos.my_pawns),
            en_passant};
}

// Negates results for negamax
SearchResult negate_search_result(SearchResult result)
{
    return {-result.upper_bound, -result.lower_bound};
}

} // anon namespace
