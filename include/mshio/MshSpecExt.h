#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mshio {
// Custom-section format integers are host-independent; vector sizes and indices are host size_t.


struct NanoSplineFormat
{
    std::string version = "1.0";
};

struct Curve
{
    std::uint64_t curve_tag = 0;
    std::uint64_t curve_type = 0;
    std::uint64_t curve_degree = 0;
    std::uint64_t num_control_points = 0;
    std::uint64_t num_knots = 0;
    std::uint64_t with_weights = 0;
    std::vector<double> data;
};

struct Patch
{
    std::uint64_t patch_tag = 0;
    std::uint64_t patch_type = 0;
    std::uint64_t degree_u = 0;
    std::uint64_t degree_v = 0;
    std::uint64_t num_control_points = 0;
    std::uint64_t num_u_knots = 0;
    std::uint64_t num_v_knots = 0;
    std::uint64_t with_weights = 0;
    std::vector<double> data;
};

}
