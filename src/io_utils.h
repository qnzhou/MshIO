#pragma once

#include <istream>
#include <limits>

namespace mshio {

void eat_white_space(std::istream& in, size_t count = std::numeric_limits<size_t>::max());

} // namespace mshio

