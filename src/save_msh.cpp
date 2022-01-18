#include <mshio/MshSpec.h>

#include "save_msh_curves.h"
#include "save_msh_data.h"
#include "save_msh_elements.h"
#include "save_msh_entities.h"
#include "save_msh_format.h"
#include "save_msh_nodes.h"
#include "save_msh_patches.h"
#include "save_msh_physical_groups.h"
#include "save_msh_nanospline_format.h"

#include <cassert>
#include <fstream>

namespace mshio {

void save_msh(std::ostream& out, const MshSpec& spec)
{
    save_mesh_format(out, spec);
    if (spec.physical_groups.size() > 0) {
        save_physical_groups(out, spec);
    }
    if (!spec.entities.empty()) {
        save_entities(out, spec);
    }
    if (spec.nodes.num_nodes > 0) {
        save_nodes(out, spec);
    }
    if (spec.elements.num_elements > 0) {
        save_elements(out, spec);
    }
    if (spec.node_data.size() > 0) {
        save_node_data(out, spec);
    }
    if (spec.element_data.size() > 0) {
        save_element_data(out, spec);
    }
    if (spec.element_node_data.size() > 0) {
        save_element_node_data(out, spec);
    }
#ifdef MSHIO_EXT_NANOSPLINE
    save_nanospline_format(out, spec);
    if (spec.curves.size() > 0) {
        save_curves(out, spec);
    }
    if (spec.patches.size() > 0) {
        save_patches(out, spec);
    }
#endif
}

void save_msh(const std::string& filename, const MshSpec& spec)
{
    std::ofstream fout(filename.c_str(), std::ios::binary);
    if (!fout.is_open()) {
        throw std::runtime_error("Unable to open output file to write!");
    }
    save_msh(fout, spec);
}

} // namespace mshio
