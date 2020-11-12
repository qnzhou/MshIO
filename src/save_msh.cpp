#include <MshIO/MshSpec.h>

#include "save_msh_curves.h"
#include "save_msh_data.h"
#include "save_msh_elements.h"
#include "save_msh_format.h"
#include "save_msh_nodes.h"
#include "save_msh_patches.h"

#include <cassert>
#include <fstream>

namespace mshio {

void save_msh(std::ostream& out, const MshSpec& spec)
{
    save_mesh_format(out, spec);
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
    assert(fout.is_open());
    save_msh(fout, spec);
}

} // namespace mshio
