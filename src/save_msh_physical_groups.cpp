#include "save_msh_physical_groups.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>


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
        write_quoted_string(out, group.name);
        out << std::endl;
    }
    out << "$EndPhysicalNames" << std::endl;
}

} // namespace mshio
