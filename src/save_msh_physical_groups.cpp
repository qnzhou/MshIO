#include "save_msh_physical_groups.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <iomanip>

namespace mshio {

void save_physical_groups(std::ostream& out, const MshSpec& spec)
{
    out << "$PhysicalNames" << std::endl;
    const auto& groups = spec.physical_groups;
    checked_int32(groups.size());
    out << groups.size() << std::endl;

    for (const auto& group: groups) {
        out << group.dim << " ";
        out << group.tag << " ";
        out << std::quoted(group.name) << std::endl;
    }
    out << "$EndPhysicalNames" << std::endl;
}

} // namespace mshio
