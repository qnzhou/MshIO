#include "io_utils.h"

#include <istream>

namespace mshio {

void eat_white_space(std::istream& in, size_t count)
{
    char ch = static_cast<char>(in.peek());
    while (count > 0) {
        if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
            in.get();
            ch = static_cast<char>(in.peek());
        } else {
            break;
        }
        count--;
    }
}

} // namespace mshio

