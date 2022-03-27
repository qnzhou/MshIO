#include "load_msh_post_process.h"
#include "element_utils.h"

#include <mshio/MshSpec.h>

#include <cassert>
#include <limits>

namespace mshio {
namespace v22 {

void regroup_nodes_into_blocks(MshSpec& spec)
{
    auto& nodes = spec.nodes;
    auto& elements = spec.elements;

    std::vector<size_t> entity_dims(nodes.max_node_tag - nodes.min_node_tag + 1, 0);
    std::vector<size_t> entity_tags(nodes.max_node_tag - nodes.min_node_tag + 1, 0);
    auto node_index = [&](size_t node_tag) {
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

    std::vector<NodeBlock> node_blocks;
    node_blocks.reserve(16);

    constexpr size_t INVALID = std::numeric_limits<size_t>::max();
    size_t curr_dim = INVALID, curr_tag = INVALID;
    for (auto& block : nodes.entity_blocks) {
        for (size_t i = 0; i < block.num_nodes_in_block; i++) {
            size_t idx = node_index(block.tags[i]);
            size_t dim = entity_dims[idx];
            size_t tag = entity_tags[idx];

            if (dim != curr_dim || tag != curr_tag) {
                node_blocks.emplace_back();
                auto& curr_block = node_blocks.back();
                curr_block.tags.reserve(block.num_nodes_in_block);
                curr_block.data.reserve(block.num_nodes_in_block * 3);
                curr_block.entity_dim = static_cast<int>(dim);
                curr_block.entity_tag = static_cast<int>(tag);
                curr_dim = dim;
                curr_tag = tag;
            }

            assert(!node_blocks.empty());
            auto& curr_block = node_blocks.back();
            curr_block.tags.push_back(block.tags[i]);
            curr_block.data.push_back(block.data[i * 3]);
            curr_block.data.push_back(block.data[i * 3 + 1]);
            curr_block.data.push_back(block.data[i * 3 + 2]);
        }
    }

    for (auto& block : node_blocks) {
        block.num_nodes_in_block = block.tags.size();
    }
    std::swap(spec.nodes.entity_blocks, node_blocks);
    spec.nodes.num_entity_blocks = spec.nodes.entity_blocks.size();
}

void regroup_elements_into_blocks(MshSpec& spec)
{
    auto& elements = spec.elements;

    std::vector<ElementBlock> element_blocks;
    element_blocks.reserve(elements.num_entity_blocks);

    size_t curr_dim = 0, curr_tag = 0, curr_element_type = 0;
    for (auto& block : elements.entity_blocks) {
        if (block.entity_dim != curr_dim || block.entity_tag != curr_tag ||
            block.element_type != curr_element_type) {
            element_blocks.emplace_back();
            auto& curr_block = element_blocks.back();
            curr_block.entity_dim = block.entity_dim;
            curr_block.entity_tag = block.entity_tag;
            curr_block.element_type = block.element_type;

            curr_dim = block.entity_dim;
            curr_tag = block.entity_tag;
            curr_element_type = block.element_type;
        }

        auto& curr_block = element_blocks.back();
        curr_block.num_elements_in_block += block.num_elements_in_block;
        curr_block.data.insert(curr_block.data.end(), block.data.begin(), block.data.end());
    }

    std::swap(spec.elements.entity_blocks, element_blocks);
    spec.elements.num_entity_blocks = spec.elements.entity_blocks.size();
}

} // namespace v22

void load_msh_post_process(MshSpec& spec)
{
    if (spec.mesh_format.version == "2.2") {
        v22::regroup_nodes_into_blocks(spec);
        v22::regroup_elements_into_blocks(spec);
    }
}

} // namespace mshio
