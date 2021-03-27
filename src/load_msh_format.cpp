#include "load_msh_format.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

namespace mshio {

void load_mesh_format(std::istream& in, MshSpec& spec)
{
    MeshFormat& format = spec.mesh_format;
    in >> format.version;

    if (format.version != "2.2" && format.version != "4.1") {
        std::stringstream msg;
        msg << "Unsupported MSH version: " << format.version;
        throw UnsupportedFeature(msg.str());
    }

    in >> format.file_type;
    in >> format.data_size;

    if (format.version == "4.1" && sizeof(size_t) != format.data_size) {
        std::stringstream msg;
        msg << "MSH file (v4.1) requested data size of " << format.data_size
            << " bytes, which is different than `size_t` (" << sizeof(size_t) << " bytes)";
        throw UnsupportedFeature(msg.str());
    }

    if (format.file_type != 0) {
        int one = 0;
        eat_white_space(in);
        in.read(reinterpret_cast<char*>(&one), sizeof(int));
        if (one != 1) {
            throw UnsupportedFeature(
                "MSH file (v" + format.version + ") is encoded with unsupported endianness!");
        }
    }
}

} // namespace mshio
