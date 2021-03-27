#include "save_msh_curves.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>

namespace mshio {

namespace {

void save_curves_ascii(std::ostream& out, const MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& curves = spec.curves;
    out << curves.size() << std::endl;

    for (const auto& curve : curves) {
        out << curve.curve_tag << " ";
        out << curve.curve_type << " ";
        out << curve.curve_degree << " ";
        out << curve.num_control_points << " ";
        out << curve.num_knots << " ";
        out << curve.with_weights << std::endl;

        const size_t dim = (curve.with_weights > 0) ? 4 : 3;
        const size_t num_entries = curve.num_control_points * dim + curve.num_knots;
        assert(num_entries == curve.data.size());

        for (size_t i = 0; i < curve.num_control_points; i++) {
            for (size_t j = 0; j < dim; j++) {
                out << curve.data[i * dim + j];
                if (j + 1 == dim) {
                    out << std::endl;
                } else {
                    out << " ";
                }
            }
        }

        for (size_t i = 0; i < curve.num_knots; i++) {
            out << curve.data[curve.num_control_points * dim + i] << std::endl;
        }
    }
#endif
}

void save_curves_binary(std::ostream& out, const MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& curves = spec.curves;
    out << curves.size() << std::endl;

    for (const auto& curve : curves) {
        out << curve.curve_tag << " ";
        out << curve.curve_type << " ";
        out << curve.curve_degree << " ";
        out << curve.num_control_points << " ";
        out << curve.num_knots << " ";
        out << curve.with_weights << std::endl;

        const size_t dim = (curve.with_weights > 0) ? 4 : 3;
        const size_t num_entries = curve.num_control_points * dim + curve.num_knots;
        assert(num_entries == curve.data.size());

        out.write(reinterpret_cast<const char*>(curve.data.data()), sizeof(double) * num_entries);
    }
#endif
}

} // namespace

void save_curves(std::ostream& out, const MshSpec& spec)
{
    const bool is_ascii = spec.mesh_format.file_type == 0;

    out << "$Curves" << std::endl;
    if (is_ascii) {
        save_curves_ascii(out, spec);
    } else {
        save_curves_binary(out, spec);
    }
    out << "$EndCurves" << std::endl;
}

} // namespace mshio
