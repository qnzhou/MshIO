#pragma once

#include <cstdint>
#include <mshio/exception.h>

#include <sstream>

namespace mshio {

void assert_element_is_supported(std::int32_t element_type);

size_t nodes_per_element(std::int32_t element_type);

std::int32_t get_element_dim(std::int32_t element_type);

} // namespace mshio
