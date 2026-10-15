#include "load_msh_elements.h"
#include <mshio/MshSpec.h>
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include "element_utils.h"
#include "io_utils.h"

namespace mshio {
namespace {
template <typename Entity>
void create_entities(
    const std::map<std::int32_t, std::set<std::int32_t>>& tags,
    std::vector<Entity>& entities)
{
    for (const auto& entry : tags) {
        checked_count(entities, checked_sum(entities.size(), 1));
        entities.emplace_back();
        auto& entity = entities.back();
        entity.tag = entry.first;
        entity.physical_group_tags.assign(entry.second.begin(), entry.second.end());
    }
}
} // namespace

void load_elements(std::istream& in, MshSpec& spec)
{
    auto& elements = spec.elements;
    const bool binary = spec.mesh_format.file_type == 1;
    if (spec.mesh_format.version == "4.1") {
        if (binary) eat_white_space(in, 1);
        read_field(in, elements.num_entity_blocks, binary);
        read_field(in, elements.num_elements, binary);
        read_field(in, elements.min_element_tag, binary);
        read_field(in, elements.max_element_tag, binary);
        std::uint64_t total = 0;
        read_records(elements.entity_blocks, elements.num_entity_blocks, [&](ElementBlock& block) {
            read_field(in, block.entity_dim, binary);
            read_field(in, block.entity_tag, binary);
            read_field(in, block.element_type, binary);
            read_field(in, block.num_elements_in_block, binary);
            if (block.entity_dim < 0 || block.entity_dim > 3)
                throw InvalidFormat("Invalid MSH element dimension");
            total = checked_sum(total, block.num_elements_in_block);
            if (total > elements.num_elements)
                throw InvalidFormat("Inconsistent MSH element count");
            const auto stride = nodes_per_element(block.element_type) + 1;
            read_values(
                in,
                block.data,
                checked_product(block.num_elements_in_block, stride),
                binary);
        });
        if (total != elements.num_elements) throw InvalidFormat("Inconsistent MSH element count");
    } else if (spec.mesh_format.version == "2.2") {
        std::uint64_t count;
        read_ascii(in, count);
        checked_int32(count);
        checked_count(elements.entity_blocks, checked_sum(elements.entity_blocks.size(), count));
        if (elements.entity_blocks.empty()) {
            elements.min_element_tag = std::numeric_limits<std::uint64_t>::max();
            elements.max_element_tag = 0;
        }
        if (binary) eat_white_space(in, 1);
        std::array<std::map<std::int32_t, std::set<std::int32_t>>, 4> physical_tags;
        std::vector<std::int32_t> tags, node_ids;
        std::uint64_t processed = 0;
        while (processed < count) {
            std::int32_t type, block_count = 1, num_tags, id = 0;
            if (binary) {
                read_binary(in, &type);
                read_binary(in, &block_count);
                read_binary(in, &num_tags);
            } else {
                read_ascii(in, id);
                read_ascii(in, type);
                read_ascii(in, num_tags);
            }
            if (block_count <= 0 || nonnegative(block_count) > count - processed)
                throw InvalidFormat("Inconsistent MSH 2.2 element block count");
            const auto n = nodes_per_element(type);
            for (std::int32_t i = 0; i < block_count; ++i) {
                if (binary) read_binary(in, &id);
                read_values(in, tags, nonnegative(num_tags), binary);
                read_values(in, node_ids, n, binary);
                elements.entity_blocks.emplace_back();
                auto& block = elements.entity_blocks.back();
                block.entity_dim = get_element_dim(type);
                block.element_type = type;
                block.entity_tag = tags.size() > 1 ? tags[1] : 0;
                tags.resize(std::max<size_t>(2, tags.size()), 0);
                auto& groups = physical_tags[block.entity_dim][block.entity_tag];
                if (tags[0] != 0) groups.insert(tags[0]);
                block.legacy_tags = std::move(tags);
                block.num_elements_in_block = 1;
                block.data.resize(n + 1);
                block.data[0] = nonnegative(id);
                for (size_t j = 0; j < n; ++j) block.data[j + 1] = nonnegative(node_ids[j]);
                elements.min_element_tag = std::min(elements.min_element_tag, block.data[0]);
                elements.max_element_tag = std::max(elements.max_element_tag, block.data[0]);
                ++processed;
            }
        }
        elements.num_elements = checked_sum(elements.num_elements, count);
        elements.num_entity_blocks = elements.entity_blocks.size();
        create_entities(physical_tags[0], spec.entities.points);
        create_entities(physical_tags[1], spec.entities.curves);
        create_entities(physical_tags[2], spec.entities.surfaces);
        create_entities(physical_tags[3], spec.entities.volumes);
    } else
        throw UnsupportedFeature("Unsupported MSH version: " + spec.mesh_format.version);
}
} // namespace mshio
