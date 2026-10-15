#include "save_msh_curves.h"
#include "io_utils.h"

namespace mshio {
void save_curves(std::ostream& out, const MshSpec& spec)
{
    out << "$Curves\n";
#ifdef MSHIO_EXT_NANOSPLINE
    const bool binary = spec.mesh_format.file_type == 1;
    out << spec.curves.size() << '\n';
    for (const auto& curve : spec.curves) {
        const auto entries = checked_sum(
            checked_product(curve.num_control_points, curve.with_weights > 0 ? 4 : 3),
            curve.num_knots);
        if (entries != curve.data.size()) throw InvalidFormat("Inconsistent curve data size");
        checked_bytes(entries, sizeof(double));
        out << curve.curve_tag << ' ' << curve.curve_type << ' ' << curve.curve_degree << ' '
            << curve.num_control_points << ' ' << curve.num_knots << ' ' << curve.with_weights
            << '\n';
        if (binary) {
            write_binary(out, curve.data.data(), entries);
            out << '\n';
        } else
            for (auto value : curve.data) out << value << '\n';
    }
#else
    (void)spec;
#endif
    out << "$EndCurves\n";
    if (!out) throw InvalidFormat("Failed to write curves");
}
} // namespace mshio
