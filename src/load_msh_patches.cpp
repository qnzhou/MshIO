#include "load_msh_patches.h"
#include "io_utils.h"

namespace mshio {

namespace {

void load_patches_ascii(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    auto& patches = spec.patches;
    size_t num_patches;
    in >> num_patches;
    patches.resize(num_patches);

    for (size_t i = 0; i < num_patches; i++) {
        auto& patch = patches[i];
        in >> patch.patch_tag;
        in >> patch.patch_type;
        in >> patch.degree_u;
        in >> patch.degree_v;
        in >> patch.num_control_points;
        in >> patch.num_u_knots;
        in >> patch.num_v_knots;
        in >> patch.with_weights;

        const size_t dim = (patch.with_weights > 0) ? 4 : 3;
        const size_t num_entries =
            patch.num_control_points * dim + patch.num_u_knots + patch.num_v_knots;

        patch.data.resize(num_entries);
        for (size_t j = 0; j < num_entries; j++) {
            in >> patch.data[j];
        }
    }
#endif
}

void load_patches_binary(std::istream& in, MshSpec& spec)
{
#ifdef MSHIO_EXT_NANOSPLINE
    auto& patches = spec.patches;
    size_t num_patches;
    in >> num_patches;
    patches.resize(num_patches);

    for (size_t i = 0; i < num_patches; i++) {
        auto& patch = patches[i];
        in >> patch.patch_tag;
        in >> patch.patch_type;
        in >> patch.degree_u;
        in >> patch.degree_v;
        in >> patch.num_control_points;
        in >> patch.num_u_knots;
        in >> patch.num_v_knots;
        in >> patch.with_weights;

        const size_t dim = (patch.with_weights > 0) ? 4 : 3;
        const size_t num_entries =
            patch.num_control_points * dim + patch.num_u_knots + patch.num_v_knots;

        patch.data.resize(num_entries);
        eat_white_space(in, 1);

        in.read(reinterpret_cast<char*>(patch.data.data()), sizeof(double) * num_entries);
    }
#endif
}

} // namespace

void load_patches(std::istream& in, MshSpec& spec)
{
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (is_ascii) {
        load_patches_ascii(in, spec);
    } else {
        load_patches_binary(in, spec);
    }
}

} // namespace mshio
