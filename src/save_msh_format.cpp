#include "save_msh_format.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>

#include <ostream>

namespace mshio {

void save_mesh_format(std::ostream& out, const MshSpec& spec)
{
    const MeshFormat& format = spec.mesh_format;
    if (format.version != "4.1" && format.version != "2.2")
        throw UnsupportedFeature("Unsupported MSH version: " + format.version);
    if (format.version == "4.1" && format.data_size != 8)
        throw UnsupportedFeature("MSH 4.1 requires an eight-byte data size");
    if (format.file_type != 0 && format.file_type != 1)
        throw InvalidFormat("Invalid MSH file type");
    out << "$MeshFormat" << std::endl;
    out << format.version << " " << format.file_type << " " << format.data_size << std::endl;
    if (format.file_type == 1) {
        constexpr std::int32_t one = 1;
        write_binary(out, &one);
        out << '\n';
    }
    out << "$EndMeshFormat" << std::endl;
}

} // namespace mshio
