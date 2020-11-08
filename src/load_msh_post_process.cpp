#include "load_msh_post_process.h"
#include "element_utils.h"

#include <MshIO/MshSpec.h>

namespace mshio {
namespace v22 {

void load_msh_post_process(MshSpec& spec)
{
    auto& nodes = spec.nodes;
    auto& elements = spec.elements;

    std::vector<size_t> entity_dims(nodes.max_node_tag - nodes.min_node_tag + 1, 0);
    std::vector<size_t> entity_tags(nodes.max_node_tag - nodes.min_node_tag + 1, 0);
    auto node_index = [&](int node_tag) {
        return static_cast<size_t>(node_tag - nodes.min_node_tag);
    };

    for (const auto& block : elements.entity_blocks) {
        const auto n = nodes_per_element(block.element_type);
        for (size_t i = 0; i < block.num_elements_in_block; i++) {
            for (size_t j = 0; j < n; j++) {
                size_t idx = node_index(block.data[i * (n + 1) + j + 1]);
                entity_dims[idx] = block.entity_dim;
                entity_tags[idx] = block.entity_tag;
            }
        }
    }

    for (auto& block : nodes.entity_blocks) {
        if (block.tags.size() == 0) continue;

        block.entity_dim = entity_dims[node_index(block.tags.front())];
        block.entity_tag = entity_tags[node_index(block.tags.front())];
    }
}

} // namespace v22

void load_msh_post_process(MshSpec& spec)
{
    if (spec.mesh_format.version == "2.2") {
        v22::load_msh_post_process(spec);
    }
}

} // namespace mshio
