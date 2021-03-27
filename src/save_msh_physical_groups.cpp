#include "save_msh_physical_groups.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <iomanip>

namespace mshio {

void save_physical_groups(std::ostream& out, const MshSpec& spec)
{
    out << "$PhysicalNames" << std::endl;
    const auto& groups = spec.physical_groups;
    out << groups.size() << std::endl;

    for (const auto& group: groups) {
        out << group.dim << " ";
        out << group.tag << " ";
        out << std::quoted(group.name) << std::endl;
    }
    out << "$EndPhysicalNames" << std::endl;
}

} // namespace mshio
