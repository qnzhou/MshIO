#include "io_utils.h"

#include <istream>

namespace mshio {

long& input_byte_swap(std::ios_base& stream)
{
    static const int index = std::ios_base::xalloc();
    return stream.iword(index);
}

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
