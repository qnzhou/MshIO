#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/save_msh_format.h>
#include <MshIO/save_msh_nodes.h>

#include <cassert>
#include <fstream>

namespace MshIO {

inline void save_msh(std::ostream& out, const MshSpec& spec)
{
    save_mesh_format(out, spec);
    if (spec.nodes.total_num_nodes > 0) {
        save_nodes(out, spec);
    }
}

inline void save_msh(const std::string& filename, const MshSpec& spec)
{
    std::ofstream fout(filename.c_str(), std::ios::binary);
    assert(fout.is_open());
    save_msh(fout, spec);
}

} // namespace MshIO
