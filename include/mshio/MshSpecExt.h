#pragma once

#include <string>
#include <vector>

namespace mshio {

struct NanoSplineFormat
{
    std::string version = "1.0";
};

struct Curve
{
    size_t curve_tag = 0;
    size_t curve_type = 0;
    size_t curve_degree = 0;
    size_t num_control_points = 0;
    size_t num_knots = 0;
    size_t with_weights = 0;
    std::vector<double> data;
};

struct Patch
{
    size_t patch_tag = 0;
    size_t patch_type = 0;
    size_t degree_u = 0;
    size_t degree_v = 0;
    size_t num_control_points = 0;
    size_t num_u_knots = 0;
    size_t num_v_knots = 0;
    size_t with_weights = 0;
    std::vector<double> data;
};

}
