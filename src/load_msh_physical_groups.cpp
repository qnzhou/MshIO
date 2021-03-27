#include "load_msh_physical_groups.h"

#include <mshio/MshSpec.h>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <string>

namespace mshio {

void load_physical_groups(std::istream& in, MshSpec& spec)
{
    auto& groups = spec.physical_groups;
    int num_groups;
    in >> num_groups;
    spec.physical_groups.resize(num_groups);
    for (int i = 0; i < num_groups; i++) {
        auto& group = groups[i];
        in >> group.dim;
        in >> group.tag;
        in >> std::quoted(group.name);
    }
    assert(in.good());
}

} // namespace mshio
