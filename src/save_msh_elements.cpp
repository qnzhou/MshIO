#include "save_msh_elements.h"
#include "element_utils.h"
#include "io_utils.h"
#include <mshio/MshSpec.h>

namespace mshio {
void save_elements(std::ostream& out, const MshSpec& spec)
{
    const auto& elements = spec.elements;
    const bool binary = spec.mesh_format.file_type == 1;
    const bool v41 = spec.mesh_format.version == "4.1";
    if (!v41 && spec.mesh_format.version != "2.2")
        throw UnsupportedFeature("Unsupported MSH version: " + spec.mesh_format.version);
    if (elements.num_entity_blocks != elements.entity_blocks.size())
        throw InvalidFormat("Inconsistent MSH element block count");
    std::uint64_t total = 0;
    for (const auto& block : elements.entity_blocks) {
        const auto stride = nodes_per_element(block.element_type) + 1;
        if (block.entity_dim < 0 || block.entity_dim > 3 ||
            block.data.size() != checked_product(block.num_elements_in_block, stride))
            throw InvalidFormat("Invalid MSH element block data");
        checked_bytes(block.data.size(), sizeof(std::uint64_t));
        total = checked_sum(total, block.num_elements_in_block);
        if (!v41) {
            checked_int32(block.num_elements_in_block);
            for (auto value : block.data) checked_int32(value);
        }
    }
    if (total != elements.num_elements) throw InvalidFormat("Inconsistent MSH element count");
    if (!v41) checked_int32(elements.num_elements);
    out << "$Elements\n";
    if (v41 && binary) {
        write_binary(out, &elements.num_entity_blocks);
        write_binary(out, &elements.num_elements);
        write_binary(out, &elements.min_element_tag);
        write_binary(out, &elements.max_element_tag);
    } else if (v41) {
        out << elements.num_entity_blocks << ' ' << elements.num_elements << ' '
            << elements.min_element_tag << ' ' << elements.max_element_tag << '\n';
    } else out << elements.num_elements << '\n';
    for (const auto& block : elements.entity_blocks) {
        if (!v41 && binary && block.num_elements_in_block == 0) continue;
        const auto stride = nodes_per_element(block.element_type) + 1;
        if (v41 && binary) {
            write_binary(out, &block.entity_dim);
            write_binary(out, &block.entity_tag);
            write_binary(out, &block.element_type);
            write_binary(out, &block.num_elements_in_block);
            write_binary(out, block.data.data(), block.data.size());
        } else {
            if (v41) out << block.entity_dim << ' ' << block.entity_tag << ' '
                << block.element_type << ' ' << block.num_elements_in_block << '\n';
            if (!v41 && binary) {
                const auto count = checked_int32(block.num_elements_in_block);
                const std::int32_t num_tags = 1;
                write_binary(out, &block.element_type);
                write_binary(out, &count);
                write_binary(out, &num_tags);
            }
            const auto count = checked_size(block.num_elements_in_block);
            for (size_t i = 0; i < count; ++i) {
                if (!v41 && binary) {
                    const auto id = checked_int32(block.data[i * stride]);
                    write_binary(out, &id);
                    write_binary(out, &block.entity_tag);
                    for (size_t k = 1; k < stride; ++k) {
                        const auto node = checked_int32(block.data[i * stride + k]);
                        write_binary(out, &node);
                    }
                } else {
                    if (!v41) out << checked_int32(block.data[i * stride]) << ' '
                        << block.element_type << " 1 " << block.entity_tag << ' ';
                    for (size_t k = v41 ? 0 : 1; k < stride; ++k)
                        out << block.data[i * stride + k] << (k + 1 == stride ? '\n' : ' ');
                }
            }
        }
    }
    if (binary) out << '\n';
    out << "$EndElements\n";
    if (!out) throw InvalidFormat("Failed to write MSH elements");
}
} // namespace mshio
