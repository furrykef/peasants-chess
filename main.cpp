#include <iostream>
#include <ctime>
#include <boost/program_options.hpp>
#include "search.hpp"
#include "bitboards.hpp"

namespace po = boost::program_options;

namespace
{
    void solve();
    void perft();
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
        std::uint64_t searched_nodes = 0;
        std::time_t before, after;
        std::time(&before);
        result = search_root(max_ply, searched_nodes);
        std::time(&after);
        double time_taken = std::difftime(after, before);
        double nodes_sec = searched_nodes/time_taken;
        std::cout << "depth " << max_ply
                  << "; score (" << result.lower_bound << ", " << result.upper_bound << ")"
                  << "; nodes " << searched_nodes
                  << "; time " << time_taken
                  << "; meganodes/sec " << (nodes_sec/1'000'000)
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
        std::cout << "depth " << depth
                  << "; leaves " << perft_root(depth)
                  << std::endl;
    }
}

} // anon namespace
