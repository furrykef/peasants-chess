#include "coords.hpp"

unsigned int parse_coords(const std::string& coords)
{
    if (coords.length() != 2) {
        throw std::exception("Invalid coords");
    }

    // @TODO@ -- verify they're in range
    unsigned int col = 'h' - coords[0];
    unsigned int row = coords[1] - '1';
    return row*8 + col;
}
