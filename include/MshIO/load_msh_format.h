#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/io_utils.h>

#include <cassert>
#include <fstream>
#include <string>


namespace MshIO {

void load_mesh_format(std::istream& in, MshSpec& spec, std::ostream& log_out,
                      std::ostream& log_err) {
    MeshFormat& format = spec.mesh_format;
    in >> format.version;

    if (format.version != "2.2" && format.version != "4.1") {
        throw std::ios_base::failure("Fatal error: Msh version " +
                                     format.version + " is not supported yet!");
    }

    in >> format.file_type;
    in >> format.data_size;

    if (format.version=="4.1" && sizeof(size_t) != format.data_size) {
        log_err << "Warning: Msh requested data size (" << format.data_size
                << " bytes) is different than `size_t` (" << sizeof(size_t)
                << " bytes)";
    }

    if (format.file_type != 0) {
        int one = 0;
        eat_white_space(in);
        in.read(reinterpret_cast<char*>(&one), sizeof(int));
        if (one != 1) {
            throw std::ios_base::failure("Fatal error: endianness mismatch!");
        }
    }
}

}  // namespace MshIO
