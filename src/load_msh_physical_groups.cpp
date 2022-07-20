#include "load_msh_physical_groups.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <string>

namespace mshio {

void load_physical_groups(std::istream& in, MshSpec& spec)
{
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (!is_ascii) throw UnsupportedFeature("Parsing binary physical groups is not supported!");

    auto& groups = spec.physical_groups;
    size_t num_groups;
    in >> num_groups;
    spec.physical_groups.resize(num_groups);
    for (size_t i = 0; i < num_groups; i++) {
        auto& group = groups[i];
        in >> group.dim;
        in >> group.tag;
        in >> std::quoted(group.name);
    }
    assert(in.good());
}

} // namespace mshio
