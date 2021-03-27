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

    ASSERT(nodes.min_node_tag <= nodes.max_node_tag, "Min node tag > max node tag.");
    ASSERT(
        elements.min_element_tag <= elements.max_element_tag, "Min element tag > max element tag.");

    for (size_t i = 0; i < nodes.num_entity_blocks; i++) {
        const NodeBlock& block = nodes.entity_blocks[i];
        ASSERT(block.tags.size() == block.num_nodes_in_block, "Inconsist number of node tags.");
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            ASSERT(block.tags[j] >= nodes.min_node_tag, "Node tag < min node tag.");
            ASSERT(block.tags[j] <= nodes.max_node_tag, "Node tag > max node tag.");
        }

        if (block.parametric > 0) {
            ASSERT(block.data.size() ==
                       block.num_nodes_in_block * static_cast<size_t>(3 + block.entity_dim),
                "Invalide node data size.");
        } else {
            ASSERT(block.data.size() == block.num_nodes_in_block * 3, "Invalid node data size.");
        }
    }

    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        const ElementBlock& block = elements.entity_blocks[i];
        const size_t entries_per_element = block.data.size() / block.num_elements_in_block;
        ASSERT(block.data.size() % entries_per_element == 0, "Invalid element data size.");
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
}

} // namespace mshio
