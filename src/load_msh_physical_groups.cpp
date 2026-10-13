#include "load_msh_physical_groups.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>

#include <fstream>
#include <iomanip>
#include <string>

namespace mshio {

void load_physical_groups(std::istream& in, MshSpec& spec)
{
    std::int32_t num_groups;
    read_ascii(in, num_groups);
    read_records(spec.physical_groups, nonnegative(num_groups), [&](PhysicalGroup& group) {
        read_ascii(in, group.dim);
        read_ascii(in, group.tag);
        if (!(in >> std::quoted(group.name))) throw InvalidFormat("Truncated physical group name");
    });
}

} // namespace mshio
