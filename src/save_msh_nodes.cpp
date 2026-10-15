#include "save_msh_nodes.h"
#include <mshio/MshSpec.h>
#include "io_utils.h"

namespace mshio {
void save_nodes(std::ostream& out, const MshSpec& spec)
{
    const auto& nodes = spec.nodes;
    const bool binary = spec.mesh_format.file_type == 1;
    const bool v41 = spec.mesh_format.version == "4.1";
    if (!v41 && spec.mesh_format.version != "2.2")
        throw UnsupportedFeature("Unsupported MSH version: " + spec.mesh_format.version);
    if (nodes.num_entity_blocks != nodes.entity_blocks.size())
        throw InvalidFormat("Inconsistent MSH node block count");
    std::uint64_t total = 0;
    for (const auto& block : nodes.entity_blocks) {
        const auto stride = node_stride(block.entity_dim, block.parametric);
        if (block.tags.size() != block.num_nodes_in_block ||
            block.data.size() != checked_product(block.num_nodes_in_block, stride))
            throw InvalidFormat("Inconsistent MSH node block data");
        checked_bytes(block.tags.size(), sizeof(std::uint64_t));
        checked_bytes(block.data.size(), sizeof(double));
        total = checked_sum(total, block.num_nodes_in_block);
        if (!v41)
            for (auto tag : block.tags) checked_int32(tag);
    }
    if (total != nodes.num_nodes) throw InvalidFormat("Inconsistent MSH node count");
    if (!v41) checked_int32(nodes.num_nodes);
    out << "$Nodes\n";
    if (v41 && binary) {
        write_binary(out, &nodes.num_entity_blocks);
        write_binary(out, &nodes.num_nodes);
        write_binary(out, &nodes.min_node_tag);
        write_binary(out, &nodes.max_node_tag);
    } else if (v41) {
        out << nodes.num_entity_blocks << ' ' << nodes.num_nodes << ' ' << nodes.min_node_tag << ' '
            << nodes.max_node_tag << '\n';
    } else
        out << nodes.num_nodes << '\n';
    for (const auto& block : nodes.entity_blocks) {
        const auto stride = node_stride(block.entity_dim, block.parametric);
        if (v41 && binary) {
            write_binary(out, &block.entity_dim);
            write_binary(out, &block.entity_tag);
            write_binary(out, &block.parametric);
            write_binary(out, &block.num_nodes_in_block);
            write_binary(out, block.tags.data(), block.tags.size());
            write_binary(out, block.data.data(), block.data.size());
        } else {
            if (v41) {
                out << block.entity_dim << ' ' << block.entity_tag << ' ' << block.parametric << ' '
                    << block.num_nodes_in_block << '\n';
                for (auto tag : block.tags) out << tag << '\n';
            }
            for (size_t i = 0; i < block.tags.size(); ++i) {
                if (binary) {
                    const auto tag = checked_int32(block.tags[i]);
                    write_binary(out, &tag);
                    write_binary(out, block.data.data() + i * stride, 3);
                } else {
                    if (!v41) out << checked_int32(block.tags[i]) << ' ';
                    const auto n = v41 ? stride : 3;
                    for (size_t k = 0; k < n; ++k)
                        out << block.data[i * stride + k] << (k + 1 == n ? '\n' : ' ');
                }
            }
        }
    }
    if (binary) out << '\n';
    out << "$EndNodes\n";
    if (!out) throw InvalidFormat("Failed to write MSH nodes");
}
} // namespace mshio
