#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include "load_msh_curves.h"
#include "load_msh_data.h"
#include "load_msh_elements.h"
#include "load_msh_entities.h"
#include "load_msh_format.h"
#include "load_msh_nanospline_format.h"
#include "load_msh_nodes.h"
#include "load_msh_patches.h"
#include "load_msh_physical_groups.h"
#include "load_msh_post_process.h"
#include "msh_extra_sections_io.h"
#include "msh_topology_io.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

namespace mshio {

void forward_to(std::istream& in, const std::string& flag)
{
    std::string buf;
    while (in >> buf) {
        if (buf == flag) return;
    }
    throw InvalidFormat("Missing or truncated MSH section terminator: " + flag);
}

MshSpec load_msh(std::istream& in)
{
    MshSpec spec;
    std::string buf, end_str;

    while (in >> buf) {
        if (buf.size() == 0 || buf[0] != '$') continue;
        end_str = "$End" + buf.substr(1);
        if (buf == "$MeshFormat") {
            load_mesh_format(in, spec);
        } else if (buf == "$Entities") {
            load_entities(in, spec);
        } else if (buf == "$PartitionedEntities") {
            load_partitioned_entities(in, spec);
        } else if (buf == "$PhysicalNames") {
            load_physical_groups(in, spec);
        } else if (buf == "$Nodes") {
            load_nodes(in, spec);
        } else if (buf == "$Elements") {
            load_elements(in, spec);
        } else if (buf == "$Periodic") {
            load_periodic(in, spec);
        } else if (buf == "$GhostElements") {
            load_ghost_elements(in, spec);
        } else if (buf == "$Parametrizations") {
            load_parametrizations(in, spec);
        } else if (buf == "$InterpolationScheme") {
            load_interpolation_scheme(in, spec);
        } else if (buf == "$NodeData") {
            load_node_data(in, spec);
        } else if (buf == "$ElementData") {
            load_element_data(in, spec);
        } else if (buf == "$ElementNodeData") {
            load_element_node_data(in, spec);
        } else if (buf == "$NanoSplineFormat") {
            load_nanospline_format(in, spec);
        } else if (buf == "$Curves") {
            load_curves(in, spec);
        } else if (buf == "$Patches") {
            load_patches(in, spec);
        } else {
            std::cerr << "Warning: skipping section \"" << buf << "\"" << std::endl;
        }
        forward_to(in, end_str);
    }

    load_msh_post_process(spec);
    return spec;
}

MshSpec load_msh(const std::string& filename)
{
    std::ifstream fin(filename.c_str(), std::ios::binary);
    if (!fin.is_open()) {
        throw std::runtime_error("Input file does not exist!");
    }
    return load_msh(fin);
}

} // namespace mshio
