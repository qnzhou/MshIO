#include "save_msh_format.h"

#include <mshio/MshSpec.h>

#include <ostream>

namespace mshio {

void save_mesh_format(std::ostream& out, const MshSpec& spec)
{
    const MeshFormat& format = spec.mesh_format;
    out << "$MeshFormat" << std::endl;
    out << format.version << " " << format.file_type << " " << format.data_size << std::endl;
    if (format.file_type == 1) {
        constexpr int one = 1;
        out.write(reinterpret_cast<const char*>(&one), sizeof(int));
    }
    out << "$EndMeshFormat" << std::endl;
}

} // namespace mshio
