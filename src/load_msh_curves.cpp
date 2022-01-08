#include "load_msh_curves.h"
#include "io_utils.h"

namespace mshio {

namespace {

void load_curves_ascii(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    auto& curves = spec.curves;
    size_t num_curves;
    in >> num_curves;

    curves.resize(num_curves);
    for (size_t i = 0; i < num_curves; i++) {
        auto& curve = curves[i];
        in >> curve.curve_tag;
        in >> curve.curve_type;
        in >> curve.curve_degree;
        in >> curve.num_control_points;
        in >> curve.num_knots;
        in >> curve.with_weights;

        size_t num_entries =
            curve.num_control_points * ((curve.with_weights > 0) ? 4 : 3) + curve.num_knots;
        curve.data.resize(num_entries);
        for (size_t j = 0; j < num_entries; j++) {
            in >> curve.data[j];
        }
    }
#endif
}

void load_curves_binary(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    auto& curves = spec.curves;
    size_t num_curves;
    in >> num_curves;

    curves.resize(num_curves);
    for (size_t i = 0; i < num_curves; i++) {
        auto& curve = curves[i];
        in >> curve.curve_tag;
        in >> curve.curve_type;
        in >> curve.curve_degree;
        in >> curve.num_control_points;
        in >> curve.num_knots;
        in >> curve.with_weights;

        eat_white_space(in, 1);
        size_t num_entries =
            curve.num_control_points * ((curve.with_weights > 0) ? 4 : 3) + curve.num_knots;
        curve.data.resize(num_entries);
        in.read(reinterpret_cast<char*>(curve.data.data()), sizeof(double) * num_entries);
    }
#endif
}

} // namespace

void load_curves(std::istream& in, MshSpec& spec)
{
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (is_ascii) {
        load_curves_ascii(in, spec);
    } else {
        load_curves_binary(in, spec);
    }
}

} // namespace mshio
