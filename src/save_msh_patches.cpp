#include "save_msh_patches.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>

namespace mshio {

namespace {

void save_patches_ascii(std::ostream& out, const MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& patches = spec.patches;
    out << patches.size() << std::endl;
    for (const auto& patch : patches) {
        out << patch.patch_tag << " ";
        out << patch.patch_type << " ";
        out << patch.degree_u << " ";
        out << patch.degree_v << " ";
        out << patch.num_control_points << " ";
        out << patch.num_u_knots << " ";
        out << patch.num_v_knots << " ";
        out << patch.with_weights << std::endl;

        const size_t dim = (patch.with_weights > 0) ? 4 : 3;
        const size_t num_entries =
            patch.num_control_points * dim + patch.num_u_knots + patch.num_v_knots;
        assert(patch.data.size() == num_entries);

        for (size_t i = 0; i < patch.num_control_points; i++) {
            for (size_t j = 0; j < dim; j++) {
                out << patch.data[i * dim + j];
                if (j + 1 == dim) {
                    out << std::endl;
                } else {
                    out << ' ';
                }
            }
        }

        for (size_t i = 0; i < patch.num_u_knots; i++) {
            out << patch.data[patch.num_control_points * dim + i] << std::endl;
        }
        for (size_t i = 0; i < patch.num_v_knots; i++) {
            out << patch.data[patch.num_control_points * dim + patch.num_u_knots + i] << std::endl;
        }
    }
#endif
}

void save_patches_binary(std::ostream& out, const MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& patches = spec.patches;
    out << patches.size() << std::endl;
    for (const auto& patch : patches) {
        out << patch.patch_tag << " ";
        out << patch.patch_type << " ";
        out << patch.degree_u << " ";
        out << patch.degree_v << " ";
        out << patch.num_control_points << " ";
        out << patch.num_u_knots << " ";
        out << patch.num_v_knots << " ";
        out << patch.with_weights << std::endl;

        const size_t dim = (patch.with_weights > 0) ? 4 : 3;
        const size_t num_entries =
            patch.num_control_points * dim + patch.num_u_knots + patch.num_v_knots;
        assert(patch.data.size() == num_entries);

        out.write(reinterpret_cast<const char*>(patch.data.data()), sizeof(double) * num_entries);
    }
#endif
}

} // namespace


void save_patches(std::ostream& out, const MshSpec& spec)
{
    const bool is_ascii = spec.mesh_format.file_type == 0;

    out << "$Patches" << std::endl;
    if (is_ascii) {
        save_patches_ascii(out, spec);
    } else {
        save_patches_binary(out, spec);
    }
    out << "$EndPatches" << std::endl;
}

} // namespace mshio
