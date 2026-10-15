#include "save_msh_patches.h"
#include "io_utils.h"

namespace mshio {
void save_patches(std::ostream& out, const MshSpec& spec)
{
    out << "$Patches\n";
#ifdef MSHIO_EXT_NANOSPLINE
    const bool binary = spec.mesh_format.file_type == 1;
    out << spec.patches.size() << '\n';
    for (const auto& patch : spec.patches) {
        const auto entries = checked_sum(
            checked_product(patch.num_control_points, patch.with_weights > 0 ? 4 : 3),
            checked_sum(patch.num_u_knots, patch.num_v_knots));
        if (entries != patch.data.size()) throw InvalidFormat("Inconsistent patch data size");
        checked_bytes(entries, sizeof(double));
        out << patch.patch_tag << ' ' << patch.patch_type << ' ' << patch.degree_u << ' '
            << patch.degree_v << ' ' << patch.num_control_points << ' ' << patch.num_u_knots << ' '
            << patch.num_v_knots << ' ' << patch.with_weights << '\n';
        if (binary) {
            write_binary(out, patch.data.data(), entries);
            out << '\n';
        } else
            for (auto value : patch.data) out << value << '\n';
    }
#else
    (void)spec;
#endif
    out << "$EndPatches\n";
    if (!out) throw InvalidFormat("Failed to write patches");
}
} // namespace mshio
