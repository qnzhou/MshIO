#include "load_msh_curves.h"
#include "io_utils.h"

namespace mshio {
void load_curves(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const bool binary = spec.mesh_format.file_type == 1;
    std::uint64_t count;
    read_ascii(in, count);
    read_records(spec.curves, count, [&](Curve& curve) {
        read_ascii(in, curve.curve_tag);
        read_ascii(in, curve.curve_type);
        read_ascii(in, curve.curve_degree);
        read_ascii(in, curve.num_control_points);
        read_ascii(in, curve.num_knots);
        read_ascii(in, curve.with_weights);
        const auto entries = checked_sum(checked_product(curve.num_control_points,
            curve.with_weights > 0 ? 4 : 3), curve.num_knots);
        if (binary) eat_white_space(in, 1);
        read_values(in, curve.data, entries, binary);
    });
#else
    (void)in;
    (void)spec;
#endif
}
} // namespace mshio
