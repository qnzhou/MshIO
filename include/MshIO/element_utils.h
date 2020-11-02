#pragma once

#include <MshIO/exception.h>

#include <sstream>

namespace mshio {

inline void assert_element_is_supported(int element_type)
{
    if (element_type <= 0 || element_type >= 15) {
        std::stringstream msg;
        msg << "Unsupported element type: " << element_type;
        throw UnsupportedFeature(msg.str());
    }
}

inline size_t nodes_per_element(int element_type)
{
    assert_element_is_supported(element_type);
    // We support the first 15 element type so far.
    constexpr size_t element_sizes[15] = {0, 2, 3, 4, 4, 8, 6, 5, 3, 6, 9, 10, 27, 18, 14};
    return element_sizes[element_type];
}

inline int get_element_dim(int element_type)
{
    assert_element_is_supported(element_type);
    // We support the first 15 element type so far.
    constexpr int element_dims[15] = {0, 1, 2, 2, 3, 3, 3, 3, 1, 2, 2, 3, 3, 3, 3};
    return element_dims[element_type];
}

} // namespace mshio
