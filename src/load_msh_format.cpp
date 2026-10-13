#include "load_msh_format.h"
#include "io_utils.h"
#include <mshio/MshSpec.h>
#include <mshio/exception.h>

namespace mshio {
void load_mesh_format(std::istream& in, MshSpec& spec)
{
    auto& format = spec.mesh_format;
    if (!(in >> format.version)) throw InvalidFormat("Missing MSH version");
    if (format.version != "2.2" && format.version != "4.1")
        throw UnsupportedFeature("Unsupported MSH version: " + format.version);
    read_ascii(in, format.file_type);
    read_ascii(in, format.data_size);
    if (format.file_type != 0 && format.file_type != 1)
        throw InvalidFormat("Invalid MSH file type");
    if (format.version == "4.1" && format.data_size != 8)
        throw UnsupportedFeature("MSH 4.1 requires an eight-byte data size");
    if (format.file_type == 1) {
        std::int32_t one = 0;
        eat_white_space(in);
        read_binary(in, &one);
        if (one != 1)
            throw UnsupportedFeature("MSH file is encoded with unsupported endianness");
    }
}
} // namespace mshio
