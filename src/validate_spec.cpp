#include "element_utils.h"
#include "io_utils.h"
#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <string>

namespace mshio {

void validate_spec(const MshSpec& spec)
{
    const Nodes& nodes = spec.nodes;
    const Elements& elements = spec.elements;

    auto ASSERT = [](bool r, const std::string& msg) {
        if (!r) {
            throw CorruptData(msg);
        }
    };

    ASSERT(nodes.num_entity_blocks == nodes.entity_blocks.size(),
        "Inconsistent entity blocks in nodes.");
    ASSERT(elements.num_entity_blocks == elements.entity_blocks.size(),
        "Inconsistent entity blocks in elements.");

    ASSERT(nodes.num_nodes == 0 || nodes.min_node_tag <= nodes.max_node_tag,
        "Min node tag > max node tag.");
    ASSERT(elements.num_elements == 0 || elements.min_element_tag <= elements.max_element_tag,
        "Min element tag > max element tag.");

    std::uint64_t total_nodes = 0, total_elements = 0;

    for (const NodeBlock& block : nodes.entity_blocks) {
        total_nodes = checked_sum(total_nodes, block.num_nodes_in_block);
        ASSERT(block.tags.size() == block.num_nodes_in_block, "Inconsist number of node tags.");
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            ASSERT(block.tags[j] >= nodes.min_node_tag, "Node tag < min node tag.");
            ASSERT(block.tags[j] <= nodes.max_node_tag, "Node tag > max node tag.");
        }

        ASSERT(block.data.size() == checked_product(block.num_nodes_in_block,
            node_stride(block.entity_dim, block.parametric)), "Invalid node data size.");
    }

    ASSERT(total_nodes == nodes.num_nodes, "Inconsistent node count.");
    for (const ElementBlock& block : elements.entity_blocks) {
        total_elements = checked_sum(total_elements, block.num_elements_in_block);
        ASSERT(block.entity_dim >= 0 && block.entity_dim <= 3, "Invalid element dimension.");
        const size_t entries_per_element = nodes_per_element(block.element_type) + 1;
        ASSERT(block.data.size() == checked_product(block.num_elements_in_block,
            entries_per_element), "Invalid element data size.");
        for (size_t j = 0; j < block.data.size(); j++) {
            if (j % entries_per_element == 0) {
                ASSERT(block.data[j] >= elements.min_element_tag, "Element tag < min element tag.");
                ASSERT(block.data[j] <= elements.max_element_tag, "Element tag > max element tag.");
            } else {
                ASSERT(block.data[j] >= nodes.min_node_tag, "Node tag in element < min node tag.");
                ASSERT(block.data[j] <= nodes.max_node_tag, "Node tag in element > max node tag.");
            }
        }
    }
    ASSERT(total_elements == elements.num_elements, "Inconsistent element count.");
}

} // namespace mshio
