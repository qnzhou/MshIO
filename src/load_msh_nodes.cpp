#include "load_msh_nodes.h"
#include "io_utils.h"
#include <mshio/MshSpec.h>
#include <algorithm>
#include <limits>

namespace mshio {
void load_nodes(std::istream& in, MshSpec& spec)
{
    auto& nodes = spec.nodes;
    const bool binary = spec.mesh_format.file_type == 1;
    const bool v41 = spec.mesh_format.version == "4.1";
    if (!v41 && spec.mesh_format.version != "2.2")
        throw UnsupportedFeature("Unsupported MSH version: " + spec.mesh_format.version);
    if (v41) {
        if (binary) eat_white_space(in, 1);
        read_field(in, nodes.num_entity_blocks, binary);
        read_field(in, nodes.num_nodes, binary);
        read_field(in, nodes.min_node_tag, binary);
        read_field(in, nodes.max_node_tag, binary);
        resize_checked(nodes.entity_blocks, nodes.num_entity_blocks);
        std::uint64_t total = 0;
        for (auto& block : nodes.entity_blocks) {
            read_field(in, block.entity_dim, binary);
            read_field(in, block.entity_tag, binary);
            read_field(in, block.parametric, binary);
            read_field(in, block.num_nodes_in_block, binary);
            total = checked_sum(total, block.num_nodes_in_block);
            if (total > nodes.num_nodes) throw InvalidFormat("Inconsistent MSH node count");
            const auto stride = node_stride(block.entity_dim, block.parametric);
            const auto entries = checked_product(block.num_nodes_in_block, stride);
            checked_count(block.tags, block.num_nodes_in_block);
            checked_count(block.data, entries);
            read_values(in, block.tags, block.num_nodes_in_block, binary);
            read_values(in, block.data, entries, binary);
        }
        if (total != nodes.num_nodes) throw InvalidFormat("Inconsistent MSH node count");
    } else {
        std::uint64_t count;
        read_ascii(in, count);
        checked_int32(count);
        if (nodes.entity_blocks.empty()) {
            nodes.min_node_tag = std::numeric_limits<std::uint64_t>::max();
            nodes.max_node_tag = 0;
        }
        checked_count(nodes.entity_blocks, checked_sum(nodes.entity_blocks.size(), 1));
        nodes.entity_blocks.emplace_back();
        auto& block = nodes.entity_blocks.back();
        block.num_nodes_in_block = count;
        checked_count(block.tags, count);
        checked_count(block.data, checked_product(count, 3));
        resize_checked(block.tags, count);
        resize_checked(block.data, checked_product(count, 3));
        if (binary) eat_white_space(in, 1);
        for (size_t i = 0; i < block.tags.size(); ++i) {
            std::int32_t tag;
            read_field(in, tag, binary);
            block.tags[i] = nonnegative(tag);
            if (binary) read_binary(in, block.data.data() + i * 3, 3);
            else for (size_t k = 0; k < 3; ++k) read_ascii(in, block.data[i * 3 + k]);
            nodes.min_node_tag = std::min(nodes.min_node_tag, block.tags[i]);
            nodes.max_node_tag = std::max(nodes.max_node_tag, block.tags[i]);
        }
        nodes.num_entity_blocks = nodes.entity_blocks.size();
        nodes.num_nodes = checked_sum(nodes.num_nodes, count);
    }
}
} // namespace mshio
