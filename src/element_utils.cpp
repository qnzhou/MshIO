#include "element_utils.h"

namespace mshio {

void assert_element_is_supported(int element_type)
{
    if (element_type <= 0 || element_type >= 32) {
        std::stringstream msg;
        msg << "Unsupported element type: " << element_type;
        throw UnsupportedFeature(msg.str());
    }
}

size_t nodes_per_element(int element_type)
{
    assert_element_is_supported(element_type);
    // We support the first 15 element type so far.
    constexpr size_t element_sizes[] = {0,
        2,
        3,
        4,
        4,
        8, // 5
        6,
        5,
        3,
        6,
        9, // 10
        10,
        27,
        18,
        14,
        1, // 15
        8,
        20,
        15,
        13,
        9, // 20
        10,
        12,
        15,
        15,
        21, // 25
        4,
        5,
        6,
        20,
        35, // 30
        56};
    return element_sizes[element_type];
}

int get_element_dim(int element_type)
{
    assert_element_is_supported(element_type);
    // We support the first 15 element type so far.
    constexpr int element_dims[] = {0,
        1,
        2,
        2,
        3,
        3, // 5
        3,
        3,
        1,
        2,
        2, // 10
        3,
        3,
        3,
        3,
        0, // 15
        2,
        3,
        3,
        3,
        2, // 20
        2,
        2,
        2,
        2,
        2, // 25
        1,
        1,
        1,
        3,
        3, // 30
        3};
    return element_dims[element_type];
}

} // namespace mshio
