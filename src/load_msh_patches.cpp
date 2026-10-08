#include "load_msh_patches.h"
#include "io_utils.h"

namespace mshio {
void load_patches(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const bool binary = spec.mesh_format.file_type == 1;
    std::uint64_t count;
    read_ascii(in, count);
    resize_checked(spec.patches, count);
    for (auto& patch : spec.patches) {
        read_ascii(in, patch.patch_tag);
        read_ascii(in, patch.patch_type);
        read_ascii(in, patch.degree_u);
        read_ascii(in, patch.degree_v);
        read_ascii(in, patch.num_control_points);
        read_ascii(in, patch.num_u_knots);
        read_ascii(in, patch.num_v_knots);
        read_ascii(in, patch.with_weights);
        const auto entries = checked_sum(checked_product(patch.num_control_points,
            patch.with_weights > 0 ? 4 : 3), checked_sum(patch.num_u_knots, patch.num_v_knots));
        if (binary) eat_white_space(in, 1);
        read_values(in, patch.data, entries, binary);
    }
#else
    (void)in;
    (void)spec;
#endif
}
} // namespace mshio
