#pragma once

#include <mshio/exception.h>

#include <sstream>

namespace mshio {

void assert_element_is_supported(int element_type);

size_t nodes_per_element(int element_type);

int get_element_dim(int element_type);

} // namespace mshio
