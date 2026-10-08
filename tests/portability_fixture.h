#pragma once

#include <mshio/mshio.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>

namespace portability {

constexpr std::uint64_t sparse_element_tag = UINT64_C(1099511627787);

inline mshio::MshSpec expected_fixture(bool sparse_elements = false)
{
    using namespace mshio;
    MshSpec spec;
    spec.physical_groups = {{0, 101, "corner"}, {1, 102, "edge"},
        {2, 103, "face"}, {3, 104, "body"}};
    spec.entities.points = {{1, 0, 0, 0, {101}}, {2, 1, 0, 0, {}},
        {3, 0, 1, 0, {}}, {4, 0, 0, 1, {}}};
    spec.entities.curves = {
        {1, 0, 0, 0, 1, 0, 0, {102}, {1, -2}},
        {2, 0, 0, 0, 1, 1, 0, {}, {2, -3}},
        {3, 0, 0, 0, 0, 1, 0, {}, {3, -1}},
        {4, 0, 0, 0, 0, 0, 1, {}, {1, -4}},
        {5, 0, 0, 0, 1, 0, 1, {}, {2, -4}},
        {6, 0, 0, 0, 0, 1, 1, {}, {3, -4}}};
    spec.entities.surfaces = {
        {1, 0, 0, 0, 1, 1, 0, {103}, {1, 2, 3}},
        {2, 0, 0, 0, 1, 0, 1, {}, {1, 5, -4}},
        {3, 0, 0, 0, 1, 1, 1, {}, {2, 6, -5}},
        {4, 0, 0, 0, 0, 1, 1, {}, {3, 4, -6}}};
    spec.entities.volumes = {{1, 0, 0, 0, 1, 1, 1, {104}, {-1, 2, 3, 4}}};
    spec.nodes.num_entity_blocks = 15;
    spec.nodes.num_nodes = 7;
    spec.nodes.min_node_tag = 1;
    spec.nodes.max_node_tag = UINT64_C(281474976710665);
    for (std::int32_t dim = 0; dim <= 3; ++dim) {
        const std::int32_t counts[] = {4, 6, 4, 1};
        for (std::int32_t tag = 1; tag <= counts[dim]; ++tag) {
            NodeBlock block;
            block.entity_dim = dim;
            block.entity_tag = tag;
            spec.nodes.entity_blocks.push_back(block);
        }
    }
    auto& nodes = spec.nodes.entity_blocks.back();
    nodes.num_nodes_in_block = 7;
    nodes.tags = {1, 2, 3, 4, UINT64_C(4294967297),
        UINT64_C(1099511627783), UINT64_C(281474976710665)};
    nodes.data = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        2, 0, 0, 2, 1, 0, 2, 0, 1};
    spec.elements.num_entity_blocks = 1;
    spec.elements.num_elements = 2;
    spec.elements.min_element_tag = sparse_elements ? 13 : 11;
    spec.elements.max_element_tag = sparse_elements ? sparse_element_tag : 13;
    ElementBlock block;
    block.entity_dim = 3;
    block.entity_tag = 1;
    block.element_type = 4;
    block.num_elements_in_block = 2;
    block.data = {13, 1, 2, 3, 4, sparse_elements ? sparse_element_tag : 11,
        2, UINT64_C(4294967297), UINT64_C(1099511627783), UINT64_C(281474976710665)};
    spec.elements.entity_blocks.push_back(block);
    Data data;
    data.header.string_tags = {"node-vector"};
    data.header.real_tags = {0.5};
    data.header.int_tags = {0, 3, 2, 0};
    data.entries = {{1, 0, {1, 2, 3}}, {2, 0, {4, 5, 6}}};
    spec.node_data.push_back(data);
    data.header.string_tags = {"element-scalar"};
    data.header.int_tags = {0, 1, 1, 0};
    data.entries = {{13, 0, {7}}};
    spec.element_data.push_back(data);
    data.header.string_tags = {"element-node"};
    data.header.int_tags = {0, 2, 1, 0};
    data.entries = {{13, 4, {1, 2, 3, 4, 5, 6, 7, 8}}};
    spec.element_node_data.push_back(data);
    return spec;
}

inline void require(bool condition, const char* field)
{
    if (!condition) throw std::runtime_error(std::string("Portability mismatch: ") + field);
}

inline void same_data(const std::vector<mshio::Data>& actual,
    const std::vector<mshio::Data>& expected)
{
    require(actual.size() == expected.size(), "dataset count");
    for (size_t i = 0; i < actual.size(); ++i) {
        const auto& a = actual[i];
        const auto& b = expected[i];
        require(std::tie(a.header.string_tags, a.header.real_tags, a.header.int_tags) ==
            std::tie(b.header.string_tags, b.header.real_tags, b.header.int_tags), "data headers");
        require(a.entries.size() == b.entries.size(), "data entries");
        for (size_t j = 0; j < a.entries.size(); ++j) {
            require(std::tie(a.entries[j].tag, a.entries[j].num_nodes_per_element, a.entries[j].data) ==
                std::tie(b.entries[j].tag, b.entries[j].num_nodes_per_element, b.entries[j].data),
                "data tags/values");
        }
    }
}

template <typename Entity>
inline auto box_fields(const Entity& entity)
{
    return std::tie(entity.tag, entity.min_x, entity.min_y, entity.min_z,
        entity.max_x, entity.max_y, entity.max_z, entity.physical_group_tags);
}

inline void same(const mshio::MshSpec& actual, const mshio::MshSpec& expected)
{
    require(actual.mesh_format.version == expected.mesh_format.version, "version");
    require(actual.mesh_format.data_size == 8, "eight-byte header");
    const auto& an = actual.nodes;
    const auto& bn = expected.nodes;
    require(std::tie(an.num_entity_blocks, an.num_nodes, an.min_node_tag, an.max_node_tag) ==
        std::tie(bn.num_entity_blocks, bn.num_nodes, bn.min_node_tag, bn.max_node_tag), "node header");
    require(an.entity_blocks.size() == bn.entity_blocks.size(), "node blocks");
    for (size_t i = 0; i < an.entity_blocks.size(); ++i) {
        const auto& a = an.entity_blocks[i];
        const auto& b = bn.entity_blocks[i];
        require(std::tie(a.entity_dim, a.entity_tag, a.parametric, a.num_nodes_in_block, a.tags, a.data) ==
            std::tie(b.entity_dim, b.entity_tag, b.parametric, b.num_nodes_in_block, b.tags, b.data),
            "node tags/coordinates");
    }
    const auto& ae = actual.elements;
    const auto& be = expected.elements;
    require(std::tie(ae.num_entity_blocks, ae.num_elements, ae.min_element_tag, ae.max_element_tag) ==
        std::tie(be.num_entity_blocks, be.num_elements, be.min_element_tag, be.max_element_tag),
        "element header");
    require(ae.entity_blocks.size() == be.entity_blocks.size(), "element blocks");
    for (size_t i = 0; i < ae.entity_blocks.size(); ++i) {
        const auto& a = ae.entity_blocks[i];
        const auto& b = be.entity_blocks[i];
        require(std::tie(a.entity_dim, a.entity_tag, a.element_type, a.num_elements_in_block, a.data) ==
            std::tie(b.entity_dim, b.entity_tag, b.element_type, b.num_elements_in_block, b.data),
            "element tags/connectivity");
    }
    require(actual.physical_groups.size() == expected.physical_groups.size(), "physical groups");
    for (size_t i = 0; i < actual.physical_groups.size(); ++i) {
        const auto& a = actual.physical_groups[i];
        const auto& b = expected.physical_groups[i];
        require(std::tie(a.dim, a.tag, a.name) == std::tie(b.dim, b.tag, b.name), "physical names");
    }
    const auto& a = actual.entities;
    const auto& b = expected.entities;
    require(a.points.size() == b.points.size() && a.curves.size() == b.curves.size() &&
        a.surfaces.size() == b.surfaces.size() && a.volumes.size() == b.volumes.size(), "entities");
    for (size_t i = 0; i < a.points.size(); ++i) {
        require(std::tie(a.points[i].tag, a.points[i].x, a.points[i].y, a.points[i].z,
                    a.points[i].physical_group_tags) ==
            std::tie(b.points[i].tag, b.points[i].x, b.points[i].y, b.points[i].z,
                    b.points[i].physical_group_tags), "point entities");
    }
    for (size_t i = 0; i < a.curves.size(); ++i) {
        require(box_fields(a.curves[i]) == box_fields(b.curves[i]) &&
            a.curves[i].boundary_point_tags == b.curves[i].boundary_point_tags, "curve entities");
    }
    for (size_t i = 0; i < a.surfaces.size(); ++i) {
        require(box_fields(a.surfaces[i]) == box_fields(b.surfaces[i]) &&
            a.surfaces[i].boundary_curve_tags == b.surfaces[i].boundary_curve_tags, "surface entities");
    }
    for (size_t i = 0; i < a.volumes.size(); ++i) {
        require(box_fields(a.volumes[i]) == box_fields(b.volumes[i]) &&
            a.volumes[i].boundary_surface_tags == b.volumes[i].boundary_surface_tags, "volume entities");
    }
    same_data(actual.node_data, expected.node_data);
    same_data(actual.element_data, expected.element_data);
    same_data(actual.element_node_data, expected.element_node_data);
}

} // namespace portability
