#include "io_utils.h"

#include <istream>

namespace mshio {

void eat_white_space(std::istream& in, size_t count)
{
    while (count > 0 && in) {
        const int ch = in.peek();
        if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t') break;
        in.get();
        --count;
    }
}

} // namespace mshio

