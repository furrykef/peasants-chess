#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include "search.hpp"
#include "bitboards.hpp"

namespace po = boost::program_options;

namespace
{
    void solve();
    void perft();
    std::uint64_t now_in_microseconds();
}


int main(int argc, char *argv[])
{
    std::cout.imbue(std::locale(""));       // fancy numbers with commas

    try {
        po::options_description desc("Options");
        desc.add_options()
            ("help", "Show this message")
            ("perft", "Run in perft mode")
        ;
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << desc;
            return 0;
        } else if (vm.count("perft")) {
            perft();
        } else {
            solve();
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

void solve()
{
    SearchResult result = {-1, 1};
    for (unsigned int max_ply = 1; result.lower_bound != result.upper_bound; ++max_ply) {
        std::uint64_t before = now_in_microseconds();
        result = search_root(max_ply);
        std::uint64_t after = now_in_microseconds();
        double time_taken = (after - before) / 1'000'000.0;
        double leaves_sec = result.num_leaves/time_taken;
        std::cout << "depth " << max_ply
                  << "; score (" << result.lower_bound << ", " << result.upper_bound << ")"
                  << "; leaves " << result.num_leaves
                  << "; sec " << time_taken
                  << "; megaleaves/sec " << (leaves_sec/1'000'000)
                  << std::endl;
    }

    // If we get here, the game is solved
    switch (result.lower_bound) {
    case -1:
        std::cout << "Black wins.";
        break;

    case 0:
        std::cout << "The game is a draw.";
        break;

    case 1:
        std::cout << "White wins.";
        break;

    default:
        std::cout << "Something went horribly wrong!";
    }
    std::cout << std::endl;
}


void perft()
{
    for (unsigned int depth = 1; true; ++depth) {
        std::uint64_t before = now_in_microseconds();
        std::uint64_t leaves = perft_root(depth);
        std::uint64_t after = now_in_microseconds();
        double time_taken = (after - before) / 1'000'000.0;
        double leaves_sec = leaves/time_taken;
        std::cout << "depth " << depth
                  << "; leaves " << leaves
                  << "; sec " << time_taken
                  << "; megaleaves/sec " << (leaves_sec/1'000'000)
                  << std::endl;
    }
}


std::uint64_t now_in_microseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // anon namespace
