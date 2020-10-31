#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/load_msh_elements.h>
#include <MshIO/load_msh_format.h>
#include <MshIO/load_msh_nodes.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

namespace MshIO {

inline void forward_to(std::istream& in, const std::string& flag)
{
    std::string buf;
    while (!in.eof() && buf != flag) {
        in >> buf;
    }
}

inline MshSpec load_msh(std::istream& in)
{
    MshSpec spec;
    std::string buf, end_str;

    while (!in.eof()) {
        buf.clear();
        in >> buf;
        if (buf.size() == 0 || buf[0] != '$') continue;
        end_str = "$End" + buf.substr(1);
        if (buf == "$MeshFormat") {
            load_mesh_format(in, spec);
        } else if (buf == "$Nodes") {
            load_nodes(in, spec);
        } else if (buf == "$Elements") {
            load_elements(in, spec);
        } else {
            std::cerr << "Warning: skipping section \"" << buf << "\"" << std::endl;
        }
        forward_to(in, end_str);
    }

    return spec;
}

inline MshSpec load_msh(const std::string& filename)
{
    std::ifstream fin(filename.c_str(), std::ios::binary);
    assert(fin.is_open());
    return load_msh(fin);
}

} // namespace MshIO
