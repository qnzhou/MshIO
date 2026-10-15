#include <catch2/catch_test_macros.hpp>

#include "../src/load_msh_format.h"
#include "../src/load_msh_nodes.h"
#include <mshio/exception.h>
#include <mshio/mshio.h>

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace mshio;

enum class ByteOrder { little, big };

ByteOrder native_order()
{
    const std::uint32_t value = 1;
    unsigned char bytes[sizeof(value)];
    std::memcpy(bytes, &value, sizeof(value));
    return bytes[0] == 1 ? ByteOrder::little : ByteOrder::big;
}

ByteOrder opposite(ByteOrder order)
{
    return order == ByteOrder::little ? ByteOrder::big : ByteOrder::little;
}

// Encode the specified wire order directly, independently of production readers,
// writers and byte-swapping helpers. Only double's IEEE bit pattern is copied.
struct Wire
{
    explicit Wire(ByteOrder order_) : order(order_) {}

    void integer(std::uint64_t value, unsigned int width)
    {
        for (unsigned int i = 0; i < width; ++i) {
            const auto shift = 8 * (order == ByteOrder::little ? i : width - 1 - i);
            bytes.push_back(static_cast<char>((value >> shift) & 0xff));
        }
    }

    void i32(std::int32_t value) { integer(static_cast<std::uint32_t>(value), 4); }
    void u64(std::uint64_t value) { integer(value, 8); }
    void real(double value)
    {
        std::uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        u64(bits);
    }

    ByteOrder order;
    std::string bytes;
};

const std::int32_t entity_tag = 0x01020304;
const std::uint64_t large_node = UINT64_C(4294967306);
const std::uint64_t large_element = UINT64_C(8589934611);
const char* interpolation =
    "\"endian independent basis\"\n1\n2\n2\n2 2\n1 -2.5\n0.125 4\n"
    "1 2\n-0.75 8\n";

std::uint64_t node_tag(bool modern, unsigned int index)
{
    return (modern ? large_node : UINT64_C(16909060)) + index;
}

std::uint64_t element_tag(bool modern)
{
    return modern ? large_element : UINT64_C(16909100);
}

std::vector<double> node_values(bool modern)
{
    if (modern) return {1.25, -2.5, 0.125, 0.75, -4.5, 8.25, 16.125, -0.25};
    return {1.25, -2.5, 0.125, -4.5, 8.25, 16.125};
}

std::string mesh_header(const std::string& version, ByteOrder order)
{
    Wire marker(order);
    marker.i32(1);
    return "$MeshFormat\n" + version + " 1 8\n" + marker.bytes +
        "\n$EndMeshFormat\n";
}

std::string ascii_header(const std::string& version)
{
    return "$MeshFormat\n" + version + " 0 8\n$EndMeshFormat\n";
}

struct Section
{
    std::string name;
    std::string header;
    std::string payload;
    bool binary;

    std::string text() const
    {
        return "$" + name + "\n" + header + payload +
            (binary ? "\n" : "") + "$End" + name + "\n";
    }
};

Section nodes_section(bool modern, ByteOrder order)
{
    Wire out(order);
    if (modern) {
        out.u64(1);
        out.u64(2);
        out.u64(node_tag(true, 0));
        out.u64(node_tag(true, 1));
        out.i32(1);
        out.i32(entity_tag);
        out.i32(1);
        out.u64(2);
        out.u64(node_tag(true, 0));
        out.u64(node_tag(true, 1));
        for (auto value : node_values(true)) out.real(value);
    } else {
        const auto values = node_values(false);
        for (unsigned int i = 0; i < 2; ++i) {
            out.i32(static_cast<std::int32_t>(node_tag(false, i)));
            for (unsigned int j = 0; j < 3; ++j) out.real(values[i * 3 + j]);
        }
    }
    return {"Nodes", modern ? "" : "2\n", out.bytes, true};
}

Section elements_section(bool modern, ByteOrder order)
{
    Wire out(order);
    if (modern) {
        out.u64(1);
        out.u64(1);
        out.u64(element_tag(true));
        out.u64(element_tag(true));
        out.i32(1);
        out.i32(entity_tag);
        out.i32(1);
        out.u64(1);
        out.u64(element_tag(true));
        out.u64(node_tag(true, 0));
        out.u64(node_tag(true, 1));
    } else {
        out.i32(1); // Two-node line type, one element, six legacy tags.
        out.i32(1);
        out.i32(6);
        out.i32(static_cast<std::int32_t>(element_tag(false)));
        for (std::int32_t tag : {71, entity_tag, 2, 1, -16909060, 42}) out.i32(tag);
        out.i32(static_cast<std::int32_t>(node_tag(false, 0)));
        out.i32(static_cast<std::int32_t>(node_tag(false, 1)));
    }
    return {"Elements", modern ? "" : "1\n", out.bytes, true};
}

void signed_tags(Wire& out, std::int32_t first, std::int32_t second)
{
    out.u64(2);
    out.i32(first);
    out.i32(second);
}

Section entities_section(ByteOrder order)
{
    Wire out(order);
    for (int dim = 0; dim < 4; ++dim) out.u64(1);
    for (std::int32_t dim = 0; dim < 4; ++dim) {
        out.i32(entity_tag + dim);
        for (int i = 0; i < (dim == 0 ? 3 : 6); ++i) out.real(-2.5 + i * 1.25);
        signed_tags(out, 71 + dim, 81 + dim);
        if (dim) signed_tags(out, entity_tag + dim - 1, -16909060 - dim);
    }
    return {"Entities", "", out.bytes, true};
}

Section partitioned_section(ByteOrder order)
{
    Wire out(order);
    out.u64(large_node); // A count above uint32 without allocating that many records.
    out.u64(1);
    out.i32(16909061);
    out.i32(2);
    for (int dim = 0; dim < 4; ++dim) out.u64(1);
    for (std::int32_t dim = 0; dim < 4; ++dim) {
        out.i32(16909070 + dim);
        out.i32(dim);
        out.i32(entity_tag + dim);
        signed_tags(out, 1, 2);
        for (int i = 0; i < (dim == 0 ? 3 : 6); ++i) out.real(-1.25 + i * 0.5);
        signed_tags(out, 91 + dim, 101 + dim);
        if (dim) signed_tags(out, 16909069 + dim, -16909070 - dim);
    }
    return {"PartitionedEntities", "", out.bytes, true};
}

Section periodic_section(bool modern, ByteOrder order)
{
    if (!modern) {
        return {"Periodic", "",
            "1\n1 16909060 16909061\n"
            "Affine 1 0 0 -1.25 0 1 0 2.5 0 0 1 0 0 0 0 1\n"
            "1\n16909060 16909061\n", false};
    }
    Wire out(order);
    out.u64(1);
    out.i32(1);
    out.i32(16909060);
    out.i32(16909061);
    out.u64(16);
    for (int i = 0; i < 16; ++i)
        out.real(i == 3 ? -1.25 : i == 7 ? 2.5 : double(i % 5 == 0));
    out.u64(1);
    out.u64(node_tag(true, 0));
    out.u64(node_tag(true, 1));
    return {"Periodic", "", out.bytes, true};
}

Section ghosts_section(ByteOrder order)
{
    Wire out(order);
    out.u64(1);
    out.u64(large_element);
    out.i32(16909060);
    signed_tags(out, 16909061, 16909062);
    return {"GhostElements", "", out.bytes, true};
}

std::vector<double> surface_values()
{
    std::vector<double> values;
    for (int i = 0; i < 33; ++i) values.push_back((i - 16) * 0.125);
    return values;
}

Section parametrizations_section(ByteOrder order)
{
    Wire out(order);
    out.u64(1);
    out.u64(1);
    out.i32(16909060);
    out.u64(2);
    for (auto value : node_values(true)) out.real(value);
    out.i32(16909061);
    out.u64(3);
    out.u64(1);
    for (auto value : surface_values()) out.real(value);
    out.i32(0);
    out.i32(2);
    out.i32(1);
    return {"Parametrizations", "", out.bytes, true};
}

Section data_section(const std::string& name, ByteOrder order)
{
    Wire out(order);
    out.i32(-16909060);
    const bool element_nodes = name == "ElementNodeData";
    if (element_nodes) out.i32(2);
    out.real(-1.25);
    out.real(8.125);
    if (element_nodes) {
        out.real(0.0625);
        out.real(-32.5);
    }
    return {name,
        "2\n\"endian view\"\n\"endian independent basis\"\n2\n0.125\n-2.5\n"
        "4\n16909060\n2\n1\n-7\n", out.bytes, true};
}

std::vector<Section> fixture_sections(bool modern, ByteOrder order)
{
    std::vector<Section> sections;
    sections.push_back({"PhysicalNames", "", "1\n1 71 \"unchanged ASCII name\"\n", false});
    if (modern) {
        sections.push_back(entities_section(order));
        sections.push_back(partitioned_section(order));
    }
    sections.push_back(nodes_section(modern, order));
    sections.push_back(elements_section(modern, order));
    sections.push_back(periodic_section(modern, order));
    if (modern) {
        sections.push_back(ghosts_section(order));
        sections.push_back(parametrizations_section(order));
    }
    sections.push_back({"InterpolationScheme", "", interpolation, false});
    for (const auto* name : {"NodeData", "ElementData", "ElementNodeData"})
        sections.push_back(data_section(name, order));
    return sections;
}

std::string fixture(bool modern, ByteOrder order)
{
    std::string bytes = mesh_header(modern ? "4.1" : "2.2", order);
    for (const auto& section : fixture_sections(modern, order)) bytes += section.text();
    return bytes;
}

MshSpec load(const std::string& bytes)
{
    std::istringstream in(bytes, std::ios::in | std::ios::binary);
    return load_msh(in);
}

std::string save(const MshSpec& spec)
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    save_msh(out, spec);
    return out.str();
}

void require_data(const std::vector<Data>& datasets, bool element_nodes)
{
    REQUIRE(datasets.size() == 1);
    const auto& data = datasets.at(0);
    REQUIRE(data.header.string_tags ==
        std::vector<std::string>{"endian view", "endian independent basis"});
    REQUIRE(data.header.real_tags == std::vector<double>{0.125, -2.5});
    REQUIRE(data.header.int_tags == std::vector<std::int32_t>{16909060, 2, 1, -7});
    REQUIRE(data.entries.size() == 1);
    REQUIRE(data.entries.at(0).tag == -16909060);
    REQUIRE(data.entries.at(0).num_nodes_per_element == (element_nodes ? 2 : 0));
    const std::vector<double> expected = element_nodes ?
        std::vector<double>{-1.25, 8.125, 0.0625, -32.5} :
        std::vector<double>{-1.25, 8.125};
    REQUIRE(data.entries.at(0).data == expected);
}

void require_nodes(const MshSpec& spec, bool modern)
{
    REQUIRE(spec.nodes.num_entity_blocks == 1);
    REQUIRE(spec.nodes.num_nodes == 2);
    REQUIRE(spec.nodes.min_node_tag == node_tag(modern, 0));
    REQUIRE(spec.nodes.max_node_tag == node_tag(modern, 1));
    REQUIRE(spec.nodes.entity_blocks.size() == 1);
    const auto& block = spec.nodes.entity_blocks.at(0);
    REQUIRE(block.entity_dim == 1);
    REQUIRE(block.entity_tag == entity_tag);
    REQUIRE(block.parametric == (modern ? 1 : 0));
    REQUIRE(block.num_nodes_in_block == 2);
    REQUIRE(block.tags == std::vector<std::uint64_t>{node_tag(modern, 0), node_tag(modern, 1)});
    REQUIRE(block.data == node_values(modern));
}

void require_fixture(const MshSpec& spec, bool modern)
{
    REQUIRE(spec.mesh_format.version == (modern ? "4.1" : "2.2"));
    REQUIRE(spec.mesh_format.file_type == 1);
    REQUIRE(spec.mesh_format.data_size == 8);
    validate_spec(spec);
    require_nodes(spec, modern);
    REQUIRE(spec.elements.num_entity_blocks == 1);
    REQUIRE(spec.elements.num_elements == 1);
    REQUIRE(spec.elements.min_element_tag == element_tag(modern));
    REQUIRE(spec.elements.max_element_tag == element_tag(modern));
    const auto& element = spec.elements.entity_blocks.at(0);
    REQUIRE(element.entity_dim == 1);
    REQUIRE(element.entity_tag == entity_tag);
    REQUIRE(element.element_type == 1);
    REQUIRE(element.num_elements_in_block == 1);
    REQUIRE(element.data == std::vector<std::uint64_t>{
        element_tag(modern), node_tag(modern, 0), node_tag(modern, 1)});
    if (!modern) {
        REQUIRE(element.legacy_tags ==
            std::vector<std::int32_t>{71, entity_tag, 2, 1, -16909060, 42});
    }
    REQUIRE(spec.physical_groups.size() == 1);
    REQUIRE(spec.physical_groups.at(0).dim == 1);
    REQUIRE(spec.physical_groups.at(0).tag == 71);
    REQUIRE(spec.physical_groups.at(0).name == "unchanged ASCII name");
    require_data(spec.node_data, false);
    require_data(spec.element_data, false);
    require_data(spec.element_node_data, true);
    REQUIRE(spec.periodic_links.size() == 1);
    const auto& link = spec.periodic_links.at(0);
    REQUIRE(link.entity_dim == 1);
    REQUIRE(link.slave_tag == 16909060);
    REQUIRE(link.master_tag == 16909061);
    REQUIRE(link.affine_transform ==
        std::vector<double>{1, 0, 0, -1.25, 0, 1, 0, 2.5, 0, 0, 1, 0, 0, 0, 0, 1});
    REQUIRE(link.node_pairs.size() == 1);
    REQUIRE(link.node_pairs.at(0).slave_tag == node_tag(modern, 0));
    REQUIRE(link.node_pairs.at(0).master_tag == node_tag(modern, 1));
    REQUIRE(spec.interpolation_schemes.size() == 1);
    const auto& scheme = spec.interpolation_schemes.at(0);
    REQUIRE(scheme.name == "endian independent basis");
    REQUIRE(scheme.topologies.size() == 1);
    const auto& topology = scheme.topologies.at(0);
    REQUIRE(topology.element_topology == 2);
    REQUIRE(topology.matrices.size() == 2);
    REQUIRE(topology.matrices.at(0).num_rows == 2);
    REQUIRE(topology.matrices.at(0).num_columns == 2);
    REQUIRE(topology.matrices.at(0).data == std::vector<double>{1, -2.5, 0.125, 4});
    REQUIRE(topology.matrices.at(1).num_rows == 1);
    REQUIRE(topology.matrices.at(1).num_columns == 2);
    REQUIRE(topology.matrices.at(1).data == std::vector<double>{-0.75, 8});
    if (!modern) return;

    REQUIRE(spec.entities.points.size() == 1);
    REQUIRE(spec.entities.curves.size() == 1);
    REQUIRE(spec.entities.surfaces.size() == 1);
    REQUIRE(spec.entities.volumes.size() == 1);
    REQUIRE(spec.entities.points.at(0).tag == entity_tag);
    REQUIRE(spec.entities.points.at(0).x == -2.5);
    REQUIRE(spec.entities.points.at(0).y == -1.25);
    REQUIRE(spec.entities.points.at(0).z == 0);
    REQUIRE(spec.entities.points.at(0).physical_group_tags == std::vector<std::int32_t>{71, 81});
    REQUIRE(spec.entities.curves.at(0).tag == entity_tag + 1);
    REQUIRE(spec.entities.curves.at(0).min_x == -2.5);
    REQUIRE(spec.entities.curves.at(0).max_z == 3.75);
    REQUIRE(spec.entities.curves.at(0).physical_group_tags == std::vector<std::int32_t>{72, 82});
    REQUIRE(spec.entities.curves.at(0).boundary_point_tags ==
        std::vector<std::int32_t>{entity_tag, -16909061});
    REQUIRE(spec.entities.surfaces.at(0).tag == entity_tag + 2);
    REQUIRE(spec.entities.surfaces.at(0).max_z == 3.75);
    REQUIRE(spec.entities.surfaces.at(0).boundary_curve_tags ==
        std::vector<std::int32_t>{entity_tag + 1, -16909062});
    REQUIRE(spec.entities.volumes.at(0).tag == entity_tag + 3);
    REQUIRE(spec.entities.volumes.at(0).max_z == 3.75);
    REQUIRE(spec.entities.volumes.at(0).boundary_surface_tags ==
        std::vector<std::int32_t>{entity_tag + 2, -16909063});
    const auto& partitions = spec.partitioned_entities;
    REQUIRE(partitions.num_partitions == large_node);
    REQUIRE(partitions.ghost_entities.size() == 1);
    REQUIRE(partitions.ghost_entities.at(0).tag == 16909061);
    REQUIRE(partitions.ghost_entities.at(0).partition == 2);
    REQUIRE(partitions.points.size() == 1);
    REQUIRE(partitions.curves.size() == 1);
    REQUIRE(partitions.surfaces.size() == 1);
    REQUIRE(partitions.volumes.size() == 1);
    const auto& point = partitions.points.at(0);
    REQUIRE(point.tag == 16909070);
    REQUIRE(point.parent_dim == 0);
    REQUIRE(point.parent_tag == entity_tag);
    REQUIRE(point.partitions == std::vector<std::int32_t>{1, 2});
    REQUIRE(point.x == -1.25);
    REQUIRE(point.y == -0.75);
    REQUIRE(point.z == -0.25);
    REQUIRE(point.physical_group_tags == std::vector<std::int32_t>{91, 101});
    REQUIRE(partitions.curves.at(0).tag == 16909071);
    REQUIRE(partitions.curves.at(0).parent_dim == 1);
    REQUIRE(partitions.curves.at(0).parent_tag == entity_tag + 1);
    REQUIRE(partitions.curves.at(0).partitions == std::vector<std::int32_t>{1, 2});
    REQUIRE(partitions.curves.at(0).max_z == 1.25);
    REQUIRE(partitions.curves.at(0).boundary_point_tags ==
        std::vector<std::int32_t>{16909070, -16909071});
    REQUIRE(partitions.surfaces.at(0).tag == 16909072);
    REQUIRE(partitions.surfaces.at(0).parent_dim == 2);
    REQUIRE(partitions.surfaces.at(0).parent_tag == entity_tag + 2);
    REQUIRE(partitions.surfaces.at(0).max_z == 1.25);
    REQUIRE(partitions.surfaces.at(0).boundary_curve_tags ==
        std::vector<std::int32_t>{16909071, -16909072});
    REQUIRE(partitions.volumes.at(0).tag == 16909073);
    REQUIRE(partitions.volumes.at(0).parent_dim == 3);
    REQUIRE(partitions.volumes.at(0).parent_tag == entity_tag + 3);
    REQUIRE(partitions.volumes.at(0).max_z == 1.25);
    REQUIRE(partitions.volumes.at(0).boundary_surface_tags ==
        std::vector<std::int32_t>{16909072, -16909073});
    REQUIRE(spec.ghost_elements.size() == 1);
    REQUIRE(spec.ghost_elements.at(0).element_tag == large_element);
    REQUIRE(spec.ghost_elements.at(0).partition == 16909060);
    REQUIRE(spec.ghost_elements.at(0).ghost_partitions ==
        std::vector<std::int32_t>{16909061, 16909062});
    REQUIRE(spec.parametrizations.curves.size() == 1);
    REQUIRE(spec.parametrizations.curves.at(0).tag == 16909060);
    REQUIRE(spec.parametrizations.curves.at(0).num_nodes == 2);
    REQUIRE(spec.parametrizations.curves.at(0).data == node_values(true));
    REQUIRE(spec.parametrizations.surfaces.size() == 1);
    REQUIRE(spec.parametrizations.surfaces.at(0).tag == 16909061);
    REQUIRE(spec.parametrizations.surfaces.at(0).num_nodes == 3);
    REQUIRE(spec.parametrizations.surfaces.at(0).num_triangles == 1);
    REQUIRE(spec.parametrizations.surfaces.at(0).data == surface_values());
    REQUIRE(spec.parametrizations.surfaces.at(0).triangle_indices ==
        std::vector<std::int32_t>{0, 2, 1});
}

// Leave each independently formatted stream immediately before its Nodes payload,
// permitting headers and payload reads to be genuinely interleaved.
void begin_nodes(std::istream& in, MshSpec& spec)
{
    std::string marker;
    REQUIRE(bool(in >> marker));
    REQUIRE(marker == "$MeshFormat");
    load_mesh_format(in, spec);
    REQUIRE(bool(in >> marker));
    REQUIRE(marker == "$EndMeshFormat");
    REQUIRE(bool(in >> marker));
    REQUIRE(marker == "$Nodes");
}

} // namespace

TEST_CASE("Both binary byte orders preserve MSH 2.2 and 4.1 fields and native output",
    "[io][endianness][binary]")
{
    for (bool modern : {false, true}) {
        for (auto order : {ByteOrder::little, ByteOrder::big}) {
            INFO("MSH " << (modern ? "4.1" : "2.2") << " little-endian=" <<
                (order == ByteOrder::little));
            const auto spec = load(fixture(modern, order));
            require_fixture(spec, modern);
            const auto bytes = save(spec);
            const auto header = mesh_header(modern ? "4.1" : "2.2", native_order());
            REQUIRE(bytes.compare(0, header.size(), header) == 0);
            for (const auto& expected : fixture_sections(modern, native_order())) {
                // Legacy Periodic remains ASCII but the writer normalizes spacing.
                if (!modern && expected.name == "Periodic") {
                    REQUIRE(bytes.find("Affine 1 0 0 -1.25 ") != std::string::npos);
                    continue;
                }
                INFO("Native output section " << expected.name);
                REQUIRE(bytes.find(expected.text()) != std::string::npos);
            }
            require_fixture(load(bytes), modern);
        }
    }
}

TEST_CASE("Every binary section rejects truncated payloads in either byte order",
    "[io][endianness][binary][truncated]")
{
    for (bool modern : {false, true}) {
        for (auto order : {ByteOrder::little, ByteOrder::big}) {
            for (const auto& section : fixture_sections(modern, order)) {
                if (!section.binary) continue;
                const auto prefix = mesh_header(modern ? "4.1" : "2.2", order) +
                    "$" + section.name + "\n" + section.header;
                for (size_t length = 0; length < section.payload.size(); ++length) {
                    INFO("MSH " << (modern ? "4.1" : "2.2") << " section=" << section.name <<
                        " little-endian=" << (order == ByteOrder::little) << " length=" << length);
                    REQUIRE_THROWS_AS(load(prefix + section.payload.substr(0, length)), InvalidFormat);
                }
            }
        }
    }
}

TEST_CASE("Binary MeshFormat rejects invalid and incomplete byte-order sentinels",
    "[io][endianness][binary][invalid]")
{
    for (const std::string version : {"2.2", "4.1"}) {
        for (auto order : {ByteOrder::little, ByteOrder::big}) {
            for (std::int32_t marker : {0, 2, -1, 0x01010101, 0x00010000}) {
                Wire out(order);
                out.i32(marker);
                INFO("MSH " << version << " invalid marker=" << marker);
                REQUIRE_THROWS_AS(load("$MeshFormat\n" + version + " 1 8\n" + out.bytes +
                    "\n$EndMeshFormat\n"), InvalidFormat);
            }
            Wire valid(order);
            valid.i32(1);
            for (size_t length = 0; length < valid.bytes.size(); ++length) {
                REQUIRE_THROWS_AS(load("$MeshFormat\n" + version + " 1 8\n" +
                    valid.bytes.substr(0, length)), InvalidFormat);
            }
        }
    }
}

TEST_CASE("A valid sentinel does not excuse a payload in the opposite byte order",
    "[io][endianness][binary][invalid]")
{
    for (auto order : {ByteOrder::little, ByteOrder::big}) {
        // Swapped uint64 block counts and int32 block sizes must not be accepted.
        REQUIRE_THROWS_AS(load(mesh_header("4.1", order) +
            nodes_section(true, opposite(order)).text()), InvalidFormat);
        REQUIRE_THROWS_AS(load(mesh_header("2.2", order) +
            elements_section(false, opposite(order)).text()), InvalidFormat);
    }
}

TEST_CASE("A new MeshFormat replaces byte order on a reused public input stream",
    "[io][endianness][binary][streams]")
{
    for (bool modern : {false, true}) {
        const std::string version = modern ? "4.1" : "2.2";
        std::istringstream in;
        for (auto order : {opposite(native_order()), native_order(), opposite(native_order())}) {
            in.clear();
            in.str(fixture(modern, order));
            require_fixture(load_msh(in), modern);
            in.clear();
            in.str(ascii_header(version) + "$NodeData\n0\n1\n0.125\n3\n7\n1\n1\n"
                "16909060 -2.5\n$EndNodeData\n");
            const auto ascii = load_msh(in);
            REQUIRE(ascii.mesh_format.file_type == 0);
            REQUIRE(ascii.node_data.at(0).header.real_tags == std::vector<double>{0.125});
            REQUIRE(ascii.node_data.at(0).entries.at(0).tag == 16909060);
            REQUIRE(ascii.node_data.at(0).entries.at(0).data == std::vector<double>{-2.5});
        }
        // Also encounter new headers without changing the underlying stream buffer.
        const auto bytes = mesh_header(version, opposite(native_order())) +
            data_section("NodeData", opposite(native_order())).text() +
            ascii_header(version) +
            "$ElementData\n0\n0\n3\n0\n1\n1\n16909060 2.5\n$EndElementData\n" +
            mesh_header(version, native_order()) +
            data_section("ElementNodeData", native_order()).text();
        in.clear();
        in.str(bytes);
        const auto switched = load_msh(in);
        require_data(switched.node_data, false);
        REQUIRE(switched.element_data.at(0).entries.at(0).tag == 16909060);
        REQUIRE(switched.element_data.at(0).entries.at(0).data == std::vector<double>{2.5});
        require_data(switched.element_node_data, true);
    }
}

TEST_CASE("Interleaved input streams retain independent binary byte orders",
    "[io][endianness][binary][streams]")
{
    for (bool modern : {false, true}) {
        const std::string version = modern ? "4.1" : "2.2";
        for (auto first_order : {ByteOrder::little, ByteOrder::big}) {
            std::istringstream first(mesh_header(version, first_order) +
                nodes_section(modern, first_order).text());
            std::istringstream second(mesh_header(version, opposite(first_order)) +
                nodes_section(modern, opposite(first_order)).text());
            std::istringstream ascii(ascii_header(version) +
                (modern ? "$Nodes\n1 2 4294967306 4294967307\n1 16909060 1 2\n"
                    "4294967306 4294967307\n1.25 -2.5 0.125 0.75\n-4.5 8.25 16.125 -0.25\n" :
                    "$Nodes\n2\n16909060 1.25 -2.5 0.125\n16909061 -4.5 8.25 16.125\n"));
            MshSpec a, b, c;
            begin_nodes(first, a);
            begin_nodes(second, b);
            begin_nodes(ascii, c);
            load_nodes(second, b);
            load_nodes(ascii, c);
            load_nodes(first, a);
            // Raw legacy node loading has not yet assigned entity membership.
            if (!modern) {
                REQUIRE(a.nodes.entity_blocks.at(0).entity_dim == 0);
                REQUIRE(b.nodes.entity_blocks.at(0).entity_tag == 0);
                REQUIRE(a.nodes.entity_blocks.at(0).tags ==
                    std::vector<std::uint64_t>{node_tag(false, 0), node_tag(false, 1)});
                REQUIRE(b.nodes.entity_blocks.at(0).tags == a.nodes.entity_blocks.at(0).tags);
                REQUIRE(a.nodes.entity_blocks.at(0).data == node_values(false));
                REQUIRE(b.nodes.entity_blocks.at(0).data == node_values(false));
            } else {
                require_nodes(a, true);
                require_nodes(b, true);
            }
            REQUIRE(c.nodes.entity_blocks.at(0).data == node_values(modern));
            // A fresh public consumer must not inherit either private reader's flag.
            require_fixture(load(fixture(modern, native_order())), modern);
        }
    }
}
