#include "save_msh_elements.h"
#include <mshio/MshSpec.h>
#include <algorithm>
#include <map>
#include <utility>
#include "element_utils.h"
#include "io_utils.h"


namespace mshio {
namespace {
using PhysicalTags =
    std::map<std::pair<std::int32_t, std::int32_t>, const std::vector<std::int32_t>*>;

template <typename Entity>
void index_physical_tags(PhysicalTags& tags, std::int32_t dim, const std::vector<Entity>& entities)
{
    for (const auto& entity : entities)
        tags.emplace(std::make_pair(dim, entity.tag), &entity.physical_group_tags);
}

std::int32_t physical_tag(const PhysicalTags& tags, const ElementBlock& block)
{
    const auto it = tags.find({block.entity_dim, block.entity_tag});
    if (it == tags.end()) return 0;
    std::int32_t physical = 0;
    for (auto tag : *it->second) {
        if (tag == 0) continue;
        if (physical != 0 && physical != tag)
            throw UnsupportedFeature("MSH 2.2 requires one physical tag per element; "
                                     "set ElementBlock::legacy_tags to select its physical group");
        physical = tag;
    }
    return physical;
}
} // namespace

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
            if (!block.legacy_tags.empty() && block.legacy_tags.size() < 2)
                throw InvalidFormat("MSH 2.2 legacy tags require physical and entity entries");
            checked_int32(block.legacy_tags.size());
        }
    }
    if (total != elements.num_elements) throw InvalidFormat("Inconsistent MSH element count");
    if (!v41) checked_int32(elements.num_elements);
    PhysicalTags physical_tags;
    if (!v41 && std::any_of(
                    elements.entity_blocks.begin(),
                    elements.entity_blocks.end(),
                    [](const ElementBlock& block) {
                        return block.num_elements_in_block != 0 && block.legacy_tags.empty();
                    })) {
        index_physical_tags(physical_tags, 0, spec.entities.points);
        index_physical_tags(physical_tags, 1, spec.entities.curves);
        index_physical_tags(physical_tags, 2, spec.entities.surfaces);
        index_physical_tags(physical_tags, 3, spec.entities.volumes);
    }
    out << "$Elements\n";
    if (v41 && binary) {
        write_binary(out, &elements.num_entity_blocks);
        write_binary(out, &elements.num_elements);
        write_binary(out, &elements.min_element_tag);
        write_binary(out, &elements.max_element_tag);
    } else if (v41) {
        out << elements.num_entity_blocks << ' ' << elements.num_elements << ' '
            << elements.min_element_tag << ' ' << elements.max_element_tag << '\n';
    } else
        out << elements.num_elements << '\n';
    for (const auto& block : elements.entity_blocks) {
        if (!v41 && block.num_elements_in_block == 0) continue;
        const auto stride = nodes_per_element(block.element_type) + 1;
        if (v41 && binary) {
            write_binary(out, &block.entity_dim);
            write_binary(out, &block.entity_tag);
            write_binary(out, &block.element_type);
            write_binary(out, &block.num_elements_in_block);
            write_binary(out, block.data.data(), block.data.size());
        } else {
            const std::int32_t derived_tags[] = {
                !v41 && block.legacy_tags.empty() ? physical_tag(physical_tags, block) : 0,
                block.entity_tag};
            const auto* tags = block.legacy_tags.empty() ? derived_tags : block.legacy_tags.data();
            const auto num_tags = block.legacy_tags.empty()
                                      ? std::int32_t(2)
                                      : checked_int32(block.legacy_tags.size());
            if (v41)
                out << block.entity_dim << ' ' << block.entity_tag << ' ' << block.element_type
                    << ' ' << block.num_elements_in_block << '\n';
            if (!v41 && binary) {
                const auto count = checked_int32(block.num_elements_in_block);
                // Every element in a block has the same legacy metadata.
                write_binary(out, &block.element_type);
                write_binary(out, &count);
                write_binary(out, &num_tags);
            }
            const auto count = checked_size(block.num_elements_in_block);
            for (size_t i = 0; i < count; ++i) {
                if (!v41 && binary) {
                    const auto id = checked_int32(block.data[i * stride]);
                    write_binary(out, &id);
                    write_binary(out, tags);
                    write_binary(out, &block.entity_tag);
                    write_binary(out, tags + 2, nonnegative(num_tags - 2));
                    for (size_t k = 1; k < stride; ++k) {
                        const auto node = checked_int32(block.data[i * stride + k]);
                        write_binary(out, &node);
                    }
                } else {
                    if (!v41) {
                        out << checked_int32(block.data[i * stride]) << ' ' << block.element_type
                            << ' ' << num_tags << ' ' << tags[0] << ' ' << block.entity_tag << ' ';
                        for (std::int32_t j = 2; j < num_tags; ++j) out << tags[j] << ' ';
                    }
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
