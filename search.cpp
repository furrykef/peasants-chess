#include <algorithm>
#include "search.hpp"

// The maximum number of moves that can be made in a turn (an overestimate)
const int MAX_BRANCHES = 64;


struct SearchMove
{
    Position new_pos;
    Move move;
    bool is_capture;
};

typedef boost::container::static_vector<SearchMove, MAX_BRANCHES> MoveList;


namespace
{
void gen_moves(MoveList& movelist, const Position& pos);
bool try_advance(MoveList& movelist, const Position& pos, unsigned int bitnum, unsigned int num_squares, unsigned int en_passant_bitnum);
void try_capture(MoveList& movelist, const Position& pos, unsigned int bitnum, int direction, Bitboard en_passant_bit);
void sort_moves(MoveList& movelist);
Position flip_board(const Position& pos);
SearchResult negate_search_result(SearchResult result);
}


// pv is strictly an output that will hold the principal variation of this subtree.
// It should be empty, and will remain empty if this is a leaf node
// Returned bounds are clamped at (alpha, beta)
// @XXX@ no pv stored in TT
SearchResult search_node(int depth, const Position& pos, int alpha, int beta, TranspositionTable& tt, Variation& pv)
{
    assert(pv.size() == 0);

    // Check if position is in transposition table
    // @TODO@ -- don't want to waste time hashing if TT's size is zero
    std::uint64_t hash = calc_hash(pos);

    if (!pos.my_pawns || pos.their_pawns & 0x0000'0000'0000'00ffULL) {
        // I have no pawns or an enemy pawn is on my first rank! I've lost!
        TTEntry tt_entry(pos, -1, 1, -1, -1, depth);
        tt.insert(hash, tt_entry);
        return {std::max(-1, alpha), std::min(-1, beta), 1};
    }

    /*const TTEntry* tt_entry_ptr = tt.fetch(hash, pos);
    if (tt_entry_ptr
        && tt_entry_ptr->depth >= depth
        && tt_entry_ptr->alpha <= alpha
        && tt_entry_ptr->beta >= beta) {
        return {std::max(tt_entry_ptr->lower_bound, alpha), std::min(tt_entry_ptr->upper_bound, beta), 1};
    }*/

    // @TODO@ -- perhaps check for positions that will clearly be stalemate (all files dead)

    // @TODO@ -- perhaps check for passed pawns and don't stop searching if there are any
    if (depth == 0) {
        // Result is unknown
        TTEntry tt_entry(pos, -1, 1, -1, 1, depth);
        tt.insert(hash, tt_entry);
        return {alpha, beta, 1};
    }

    MoveList movelist;
    gen_moves(movelist, pos);

    if (movelist.size() == 0) {
        // Stalemate
        TTEntry tt_entry(pos, -1, 1, 0, 0, depth);
        tt.insert(hash, tt_entry);
        return {std::clamp(0, alpha, beta), std::min(0, beta), 1};
    }

    sort_moves(movelist);

    int best_lower_bound = -1;
    int best_upper_bound = -1;
    std::uint64_t num_childrens_leaves = 0;
    int old_alpha = alpha;
    for (const SearchMove& move : movelist) {
        Variation subvariation;
        SearchResult child_result = search_node(depth - 1,
                                                flip_board(move.new_pos),
                                                -beta,
                                                -alpha,
                                                tt,
                                                subvariation);
        num_childrens_leaves += child_result.num_leaves;
        child_result = negate_search_result(child_result);      // our score is opposite of opponent's score
        best_lower_bound = std::max(best_lower_bound, child_result.lower_bound);
        // @TODO@ -- I want to change >= to just >, which would make it copy *much* less often.
        // But then no move would be stored if alpha never improves.
        // I think this would be fixable by using a starting alpha-beta window of
        // (-inf, inf) instead of (-1, 1).
        if (child_result.lower_bound >= alpha) {
            alpha = child_result.lower_bound;
            pv.resize(1);
            pv[0] = move.move;
            pv.insert(pv.end(), subvariation.begin(), subvariation.end());
        }
        if (alpha >= beta) {
            // If we don't examine all children, we can't measure the upper bound
            // So we use 1 instead
            best_upper_bound = 1;
            break;
        }
        best_upper_bound = std::max(best_upper_bound, child_result.upper_bound);
    }

    TTEntry tt_entry(pos, old_alpha, beta, best_lower_bound, best_upper_bound, depth);
    tt.insert(hash, tt_entry);
    return {std::clamp(best_lower_bound, alpha, beta),
            std::min(best_upper_bound, beta),
            num_childrens_leaves};
}


std::uint64_t perft_node(int depth, const Position& pos)
{
    if (depth == 0) {
        return 1;
    }

    MoveList movelist;
    gen_moves(movelist, pos);

    std::uint64_t leaves = 0;
    for (const SearchMove& move : movelist) {
        leaves += perft_node(depth - 1, flip_board(move.new_pos));
    }

    return leaves;
}

std::vector<PerftMove> split_perft_node(int depth, const Position& pos)
{
    std::vector<PerftMove> result;

    if (depth == 0) {
        return result;
    }

    MoveList movelist;
    gen_moves(movelist, pos);

    for (const SearchMove& move : movelist) {
        std::uint64_t num_leaves = perft_node(depth - 1, flip_board(move.new_pos));
        PerftMove perft_move = {move.move, num_leaves};
        result.push_back(perft_move);
    }

    return result;
}


namespace
{

// This function only generates moves in the order they're found; no ordering is done.
void gen_moves(MoveList& movelist, const Position& pos)
{
    for (unsigned int bitnum = 8; bitnum < 56; ++bitnum) {
        Bitboard bit = 1ULL << bitnum;
        if (pos.my_pawns & bit) {
            // We've found one of my pawns.
            // Try a one-square advance
            bool can_advance = try_advance(movelist, pos, bitnum, 1, {});

            // If successful, try a two-square advance if on second rank
            if (can_advance && bitnum < 16) {
                try_advance(movelist, pos, bitnum, 2, bitnum+8);
            }

            Bitboard en_passant_bit = pos.en_passant_bitnum ? 1ULL << pos.en_passant_bitnum.value() : 0;
            unsigned int column = bitnum % 8;       // 0 = rightmost column; 7 = leftmost
            // Don't test invalid captures (leftward capture on leftmost column, etc.)
            if (column != 7) {
                try_capture(movelist, pos, bitnum, -1, en_passant_bit);
            }
            if (column != 0) {
                try_capture(movelist, pos, bitnum, 1, en_passant_bit);
            }
        }
    }
}

// Only checks if the destination is occupied; two-square advances do not check if a pawn is in the way!
// (This should be done by only calling after checking the result of a one-square advance)
// Returns true if the square was unoccupied
bool try_advance(MoveList& movelist,
                 const Position& pos,
                 unsigned int bitnum,
                 unsigned int num_squares,
                 unsigned int new_en_passant_bitnum)
{
    unsigned int dest_bitnum = bitnum+8*num_squares;
    Bitboard all_pawns = pos.my_pawns | pos.their_pawns;
    Bitboard bit = 1ULL << bitnum;
    Bitboard dest = 1ULL << dest_bitnum;
    if (!(all_pawns & dest)) {
        // The destination is empty; we can advance
        Bitboard my_new_pawns = (pos.my_pawns | dest) & ~bit;
        SearchMove move = {{my_new_pawns, pos.their_pawns, new_en_passant_bitnum}, {bitnum, dest_bitnum}, false};
        movelist.push_back(move);
        return true;
    }
    return false;
}

void try_capture(MoveList& movelist,
                 const Position& pos,
                 unsigned int bitnum,
                 int direction,                     // -1 = leftward; 1 = rightward
                 Bitboard en_passant_bit)
{
    unsigned int dest_bitnum = bitnum + 8 - direction;
    Bitboard bit = 1ULL << bitnum;
    Bitboard dest = 1ULL << dest_bitnum;
    assert(dest != 0);
    if ((pos.their_pawns & dest) || dest == en_passant_bit) {
        // Capture is possible
        Bitboard my_new_pawns = (pos.my_pawns | dest) & ~bit;
        Bitboard captured_pawn = (dest == en_passant_bit) ? dest >> 8 : dest;
        Bitboard their_new_pawns = pos.their_pawns & ~captured_pawn;
        SearchMove move = {{my_new_pawns, their_new_pawns, {}}, {bitnum, dest_bitnum}, true};
        movelist.push_back(move);
    }
}


void sort_moves(MoveList& movelist)
{
    std::sort(movelist.begin(),
              movelist.end(),
              [](SearchMove a, SearchMove b) -> bool {
                return a.is_capture && !b.is_capture;
              });
}


// Rotates the bitboards so that my_pawns and their_pawns are switched and vertically flipped
// Equivalent to rotating by 180 degrees, except the board is horizontally mirrored
// (we do this instead of the full rotation because it's faster)
Position flip_board(const Position& pos)
{
    std::optional<unsigned int> en_passant;
    if (pos.en_passant_bitnum) {
        en_passant = pos.en_passant_bitnum.value() ^ 56;
    }
    return {vflip_bitboard(pos.their_pawns),
            vflip_bitboard(pos.my_pawns),
            en_passant};
}

// Negates results for negamax
SearchResult negate_search_result(SearchResult result)
{
    return {-result.upper_bound, -result.lower_bound, result.num_leaves};
}

} // anon namespace
