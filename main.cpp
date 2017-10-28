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
void solve(const Position& pos);
void perft(const Position& pos);
Position parse_fen(const std::string& fen);
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
            ("perft", "Run in perft mode")
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
        if (vm.count("perft")) {
            perft(pos);
        } else {
            solve(pos);
        }
    }
    catch(std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

namespace
{

void solve(const Position& pos)
{
    TranspositionTable tt(TT_BUCKETS, TT_SLOTS_PER_BUCKET);
    int lower_bound = -1;
    int upper_bound = 1;
    for (int depth = 1; lower_bound != upper_bound; ++depth) {
        std::uint64_t before = now_in_microseconds();
        SearchResult result = search_node(depth, pos, lower_bound, upper_bound, tt);
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
                  << std::endl;
    }

    // If we get here, the position is solved
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
    std::cout << std::endl;
}


void perft(const Position& pos)
{
    for (int depth = 1; true; ++depth) {
        std::uint64_t before = now_in_microseconds();
        std::uint64_t leaves = perft_node(depth, pos);
        std::uint64_t after = now_in_microseconds();
        double time_taken = (after - before) / 1'000'000.0;
        double leaves_sec = leaves/time_taken;
        std::cout << "depth " << depth
                  << "; leaves " << leaves
                  << "; sec " << time_taken
                  << "; megaleaves/sec " << (leaves_sec/1'000'000)
                  << std::endl;
        if (leaves == 0) {
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
    unsigned int en_passant = (en_passant_str != "-") ? parse_coords(en_passant_str) : NO_EN_PASSANT;

    return {white_pawns, black_pawns, en_passant};
}


std::uint64_t now_in_microseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // anon namespace
