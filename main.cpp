#include <chrono>
#include <regex>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "coords.hpp"
#include "search.hpp"
#include "tt.hpp"

namespace po = boost::program_options;

const std::size_t TT_BUCKETS = 0x10'0000;
const std::size_t TT_SLOTS_PER_BUCKET = 4;

namespace
{
void solve(const Position& pos, int start_depth, int max_depth);
void perft(const Position& pos, int start_depth, int max_depth, bool split);
Position parse_fen(const std::string& fen);
std::string variation_to_string(const Variation& variation);
std::uint64_t now_in_microseconds();
}

const std::string START_POS = "8/XXXXXXXX/XXXXXXXX/8/8/oooooooo/oooooooo/8 -";

int main(int argc, char *argv[])
{
    init_zobrist();

    std::cout.imbue(std::locale(""));       // fancy numbers with commas

    try {
        po::options_description desc("Options");
        desc.add_options()
            ("help", "Show this message")
            ("depth", po::value<int>(), "Starting depth")
            ("max-depth", po::value<int>(), "Maximum depth")
            ("perft", "Run in perft mode")
            ("split-perft", "Run in split perft mode")
            ("pos,p", po::value<std::string>(), "Choose position to analyze")
        ;
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << desc;
            return 0;
        }
        std::string pos_fen = (vm.count("pos")) ? vm["pos"].as<std::string>() : START_POS;
        Position pos = parse_fen(pos_fen);
        int depth = (vm.count("depth")) ? vm["depth"].as<int>() : 1;
        int max_depth = (vm.count("max-depth")) ? vm["max-depth"].as<int>() : INT_MAX;
        if (vm.count("perft")) {
            perft(pos, depth, max_depth, false);
        } else if (vm.count("split-perft")) {
            perft(pos, depth, max_depth, true);
        } else {
            solve(pos, depth, max_depth);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

namespace
{

void solve(const Position& pos, int start_depth, int max_depth)
{
    TranspositionTable tt(TT_BUCKETS, TT_SLOTS_PER_BUCKET);
    int lower_bound = -1;
    int upper_bound = 1;
    for (int depth = start_depth; lower_bound != upper_bound && depth <= max_depth; ++depth) {
        Variation pv;
        std::uint64_t before = now_in_microseconds();
        SearchResult result = search_node(depth, pos, lower_bound, upper_bound, tt, pv);
        lower_bound = result.lower_bound;
        upper_bound = result.upper_bound;
        std::uint64_t after = now_in_microseconds();
        double time_taken = (after - before) / 1'000'000.0;
        double leaves_sec = result.num_leaves/time_taken;
        std::cout << "depth " << depth
                  << "; score (" << lower_bound << ", " << upper_bound << ")"
                  << "; leaves " << result.num_leaves
                  << "; sec " << time_taken
                  << "; megaleaves/sec " << (leaves_sec/1'000'000)
                  << "; pv " << variation_to_string(pv)
                  << std::endl;
    }

    if (lower_bound == upper_bound) {
        // The position is solved
        switch (lower_bound) {
        case -1:
            std::cout << "Black wins.";
            break;

        case 0:
            std::cout << "The position is drawn.";
            break;

        case 1:
            std::cout << "White wins.";
            break;

        default:
            std::cout << "Something went horribly wrong!";
        }
    } else {
        std::cout << "Maximum search depth reached.";
    }

    std::cout << std::endl;
}


void perft(const Position& pos, int start_depth, int max_depth, bool split)
{
    for (int depth = start_depth; depth <= max_depth; ++depth) {
        std::uint64_t before = now_in_microseconds();
        std::vector<PerftMove> moves = split_perft_node(depth, pos);
        std::uint64_t after = now_in_microseconds();
        std::uint64_t num_leaves = 0;
        for (const PerftMove& move : moves) {
            num_leaves += move.num_leaves;
        }
        double time_taken = (after - before) / 1'000'000.0;
        double leaves_sec = num_leaves/time_taken;
        std::cout << "depth " << depth
                  << "; leaves " << num_leaves
                  << "; sec " << time_taken
                  << "; megaleaves/sec " << (leaves_sec/1'000'000)
                  << std::endl;
        if (split) {
            for (const PerftMove& move : moves) {
                std::cout << "    "
                          << bitnum_to_coords(move.move.src_bitnum) << "-"
                          << bitnum_to_coords(move.move.dest_bitnum) << ": "
                          << move.num_leaves << " leaves"
                          << std::endl;
            }
        }
        if (num_leaves == 0) {
            break;
        }
    }
}


Position parse_fen(const std::string& fen)
{
    Bitboard white_pawns = 0;
    Bitboard black_pawns = 0;
    std::regex re("([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)/"
                  "([Xo1-8]+)"
                  " ([a-h][36]|-)");
    std::smatch match;
    if (!std::regex_match(fen, match, re)) {
        throw std::exception("Position is in invalid format");
    }
    for (int i = 1; i <= 8; ++i) {
        int num_columns = 0;
        for (char ch : match[i].str()) {
            int shift_count = 1;
            int black_bit = 0;
            int white_bit = 0;
            if (ch == 'X') {
                // It's a black pawn
                black_bit = 1;              // gets shifted into black_pawns' LSB
            } else if (ch == 'o') {
                // It's a white pawn
                white_bit = 1;
            } else {
                // It's a digit
                shift_count = ch - '0';
            }
            black_pawns = (black_pawns << shift_count) | black_bit;
            white_pawns = (white_pawns << shift_count) | white_bit;
            num_columns += shift_count;
        }
        if (num_columns != 8) {
            throw std::exception("Position is not 8x8");
        }
    }

    std::string en_passant_str = match[9].str();
    std::optional<unsigned int> en_passant;
    if (en_passant_str != "-") {
        en_passant = parse_coords(en_passant_str);
    }

    return {white_pawns, black_pawns, en_passant};
}


// @TODO@ -- result has extra space at the end
std::string variation_to_string(const Variation& variation)
{
    std::string out;
    bool white = true;
    for (const Move& move : variation) {
        unsigned int xor = white ? 0 : 56;          // black's moves need to be flipped
        out += bitnum_to_coords(move.src_bitnum ^ xor);
        out += bitnum_to_coords(move.dest_bitnum ^ xor);
        out += " ";
        white = !white;
    }
    return out;
}


std::uint64_t now_in_microseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // anon namespace
