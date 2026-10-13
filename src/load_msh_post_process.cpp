#include "load_msh_post_process.h"
#include "element_utils.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>

#include <map>
#include <utility>

namespace mshio {
namespace v22 {

void regroup_nodes_into_blocks(MshSpec& spec)
{
    auto& nodes = spec.nodes;
    auto& elements = spec.elements;

    std::map<std::uint64_t, std::pair<std::int32_t, std::int32_t>> node_entities;
    for (const auto& block : nodes.entity_blocks)
        for (auto tag : block.tags) node_entities.emplace(tag, std::make_pair(0, 0));

    for (const auto& block : elements.entity_blocks) {
        const auto n = nodes_per_element(block.element_type);
        for (size_t i = 0; i < block.num_elements_in_block; i++) {
            for (size_t j = 0; j < n; j++) {
                auto it = node_entities.find(block.data[i * (n + 1) + j + 1]);
                if (it == node_entities.end()) throw InvalidFormat("Element references unknown node");
                it->second = {block.entity_dim, block.entity_tag};
            }
        }
    }

    std::vector<NodeBlock> node_blocks;
    node_blocks.reserve(16);

    bool first = true;
    std::int32_t curr_dim = 0, curr_tag = 0;
    for (auto& block : nodes.entity_blocks) {
        for (size_t i = 0; i < block.num_nodes_in_block; i++) {
            const auto entity = node_entities.at(block.tags[i]);
            const auto dim = entity.first;
            const auto tag = entity.second;

            if (first || dim != curr_dim || tag != curr_tag) {
                checked_count(node_blocks, checked_sum(node_blocks.size(), 1));
                node_blocks.emplace_back();
                auto& curr_block = node_blocks.back();
                curr_block.entity_dim = dim;
                curr_block.entity_tag = tag;
                first = false;
                curr_dim = dim;
                curr_tag = tag;
            }

            auto& curr_block = node_blocks.back();
            checked_count(curr_block.tags, checked_sum(curr_block.tags.size(), 1));
            checked_count(curr_block.data, checked_sum(curr_block.data.size(), 3));
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
    element_blocks.reserve(checked_count(element_blocks, elements.entity_blocks.size()));

    std::int32_t curr_dim = 0, curr_tag = 0, curr_element_type = 0;
    for (auto& block : elements.entity_blocks) {
        if (element_blocks.empty() || block.entity_dim != curr_dim || block.entity_tag != curr_tag ||
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
        curr_block.num_elements_in_block = checked_sum(curr_block.num_elements_in_block,
            block.num_elements_in_block);
        checked_count(curr_block.data, checked_sum(curr_block.data.size(), block.data.size()));
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
