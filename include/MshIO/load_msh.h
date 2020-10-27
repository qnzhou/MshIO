#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/load_msh_format.h>
#include <MshIO/load_msh_nodes.h>
#include <MshIO/load_msh_elements.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

namespace MshIO {

void forward_to(std::istream& in, const std::string& flag) {
    std::string buf;
    while (!in.eof() && buf != flag) {
        in >> buf;
    }
}

MshSpec load_msh(std::istream& in, std::ostream& log_out = std::cout,
                 std::ostream& log_err = std::cerr) {
    MshSpec spec;
    std::string buf, end_str;

    while (!in.eof()) {
        buf.clear();
        in >> buf;
        if (buf.size() == 0 || buf[0] != '$') continue;
        end_str = "$End" + buf.substr(1);
        if (buf == "$MeshFormat") {
            load_mesh_format(in, spec, log_out, log_err);
        } else if (buf == "$Nodes") {
            load_nodes(in, spec, log_out, log_err);
        } else if (buf == "$Elements") {
            load_elements(in, spec, log_out, log_err);
        } else {
            std::cerr << "Warning: skipping section \"" << buf << "\""
                      << std::endl;
        }
        forward_to(in, end_str);
    }

    return spec;
}

MshSpec load_msh(const std::string& filename, std::ostream& log_out = std::cout,
                 std::ostream& log_err = std::cerr) {
    std::ifstream fin(filename.c_str(), std::ios::binary);
    assert(fin.is_open());
    return load_msh(fin, log_out, log_err);
}

}  // namespace MshIO
