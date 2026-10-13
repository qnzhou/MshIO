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
        std::uint64_t total = 0;
        read_records(nodes.entity_blocks, nodes.num_entity_blocks, [&](NodeBlock& block) {
            read_field(in, block.entity_dim, binary);
            read_field(in, block.entity_tag, binary);
            read_field(in, block.parametric, binary);
            read_field(in, block.num_nodes_in_block, binary);
            total = checked_sum(total, block.num_nodes_in_block);
            if (total > nodes.num_nodes) throw InvalidFormat("Inconsistent MSH node count");
            const auto stride = node_stride(block.entity_dim, block.parametric);
            read_values(in, block.tags, block.num_nodes_in_block, binary);
            read_values(in, block.data, checked_product(block.num_nodes_in_block, stride), binary);
        });
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
        const auto n = checked_count(block.tags, count);
        const auto entries = checked_count(block.data, checked_product(count, 3));
        if (binary) eat_white_space(in, 1);
        for (size_t i = 0; i < n; ++i) {
            std::int32_t tag;
            double xyz[3];
            read_field(in, tag, binary);
            if (binary) read_binary(in, xyz, 3);
            else for (auto& value : xyz) read_ascii(in, value);
            reserve_next(block.tags, n);
            reserve_next(block.data, entries, 3);
            block.tags.push_back(nonnegative(tag));
            block.data.insert(block.data.end(), xyz, xyz + 3);
            nodes.min_node_tag = std::min(nodes.min_node_tag, block.tags.back());
            nodes.max_node_tag = std::max(nodes.max_node_tag, block.tags.back());
        }
        nodes.num_entity_blocks = nodes.entity_blocks.size();
        nodes.num_nodes = checked_sum(nodes.num_nodes, count);
    }
}
} // namespace mshio
