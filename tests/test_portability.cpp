#include <catch2/catch_test_macros.hpp>

#include "portability_fixture.h"
#include <mshio/exception.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <type_traits>

namespace {

using namespace mshio;

static_assert(std::is_same<decltype(NodeBlock::tags)::value_type, std::uint64_t>::value,
    "Node identifiers are format uint64_t, not host size_t");
static_assert(std::is_same<decltype(ElementBlock::data)::value_type, std::uint64_t>::value,
    "Element identifiers and connectivity are format uint64_t");
static_assert(std::is_same<decltype(Nodes::num_nodes), std::uint64_t>::value, "Format counts are uint64_t");
static_assert(std::is_same<decltype(DataEntry::tag), std::int32_t>::value,
    "Postprocessing identifiers remain specification int32_t");
static_assert(std::is_same<decltype(CurveEntity::boundary_point_tags)::value_type,
    std::int32_t>::value, "Entity boundaries retain their signed 32-bit width");

bool explicit_format_error(const std::string& contents)
{
    std::istringstream in(contents);
    try {
        static_cast<void>(load_msh(in));
    } catch (const InvalidFormat&) {
        return true;
    } catch (const CorruptData&) {
        return true;
    } catch (const UnsupportedFeature&) {
        return true;
    }
    return false;
}

bool explicit_save_error(const MshSpec& spec)
{
    std::ostringstream out;
    try {
        save_msh(out, spec);
    } catch (const InvalidFormat&) {
        return true;
    } catch (const CorruptData&) {
        return true;
    } catch (const UnsupportedFeature&) {
        return true;
    }
    return false;
}

std::string ascii_section(const std::string& section, const std::string& payload)
{
    return "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n$" + section + "\n" + payload +
        "\n$End" + section + "\n";
}

template <typename T>
void binary(std::ostream& out, T value)
{
    out.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(value)));
}

std::ostringstream binary_section(const char* section)
{
    std::ostringstream out;
    out << "$MeshFormat\n4.1 1 8\n";
    binary(out, std::int32_t(1));
    out << "\n$EndMeshFormat\n$" << section << '\n';
    return out;
}

std::string serialized(MshSpec spec, std::int32_t encoding)
{
    spec.mesh_format.file_type = encoding;
    std::ostringstream out;
    save_msh(out, spec);
    return out.str();
}

std::string binary_payload(const std::string& contents, const char* section)
{
    const std::string marker = std::string("$") + section + "\n";
    const auto position = contents.find(marker);
    portability::require(position != std::string::npos, "binary section marker");
    size_t begin = position + marker.size();
    const auto end = contents.find(std::string("\n$End") + section, begin);
    portability::require(end != std::string::npos, "binary end marker");
    if (std::string(section).find("Data") != std::string::npos) {
        std::istringstream header(contents.substr(begin));
        size_t count = 0;
        header >> count;
        std::string name;
        for (size_t i = 0; i < count; ++i) header >> std::quoted(name);
        header >> count;
        double real = 0;
        for (size_t i = 0; i < count; ++i) header >> real;
        header >> count;
        std::int32_t integer = 0;
        for (size_t i = 0; i < count; ++i) header >> integer;
        header.get();
        begin += static_cast<size_t>(header.tellg());
    }
    return contents.substr(begin, end - begin);
}

class FailOnFourthSync : public std::stringbuf
{
public:
    int sync() override
    {
        ++sync_count;
        return sync_count == 4 ? -1 : std::stringbuf::sync();
    }

private:
    int sync_count = 0;
};

} // namespace

TEST_CASE("Gmsh independent 4.1 fixtures preserve complete fixed-width data", "[io][portability]")
{
    const auto expected = portability::expected_fixture();
    for (const auto* file : {"/portability_4.1_ascii.msh", "/portability_4.1_binary.msh"}) {
        CAPTURE(file);
        const auto spec = load_msh(std::string(MSHIO_DATA_DIR) + file);
        REQUIRE_NOTHROW(portability::same(spec, expected));
        validate_spec(spec);
        for (std::int32_t encoding : {0, 1}) {
            CAPTURE(encoding);
            std::istringstream in(serialized(spec, encoding));
            REQUIRE_NOTHROW(portability::same(load_msh(in), expected));
        }
    }
}

TEST_CASE("Binary wire payloads match independent Gmsh output", "[io][portability][wire]")
{
    std::ifstream file(MSHIO_DATA_DIR "/portability_4.1_binary.msh", std::ios::binary);
    REQUIRE(file.good());
    const std::string fixture((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto output = serialized(portability::expected_fixture(), 1);
    for (const auto* section : {"Entities", "Nodes", "Elements", "NodeData", "ElementData", "ElementNodeData"}) {
        CAPTURE(section);
        REQUIRE(binary_payload(output, section) == binary_payload(fixture, section));
    }
    const auto sparse = portability::expected_fixture(true);
    std::ostringstream expected_elements;
    for (const std::uint64_t count : {UINT64_C(1), UINT64_C(2), UINT64_C(13), portability::sparse_element_tag}) {
        binary(expected_elements, count);
    }
    for (const std::int32_t value : {3, 1, 4}) binary(expected_elements, value);
    binary(expected_elements, UINT64_C(2));
    for (const auto value : sparse.elements.entity_blocks.front().data) binary(expected_elements, value);
    REQUIRE(binary_payload(serialized(sparse, 1), "Elements") == expected_elements.str());
}

TEST_CASE("Sparse uint64 node and element identifiers use small allocations", "[io][portability]")
{
    auto expected = portability::expected_fixture(true);
    for (std::int32_t encoding : {0, 1}) {
        CAPTURE(encoding);
        std::istringstream in(serialized(expected, encoding));
        const auto actual = load_msh(in);
        REQUIRE_NOTHROW(portability::same(actual, expected));
        REQUIRE(actual.nodes.num_nodes == 7);
        REQUIRE(actual.elements.num_elements == 2);
        REQUIRE(actual.nodes.entity_blocks.back().tags.size() == 7);
        REQUIRE(actual.elements.entity_blocks.front().data[5] == portability::sparse_element_tag);
    }
    // Exercise the full identifier range independently of allocation counts.
    expected.nodes.max_node_tag = std::numeric_limits<std::uint64_t>::max();
    expected.nodes.entity_blocks.back().tags.back() = expected.nodes.max_node_tag;
    expected.elements.entity_blocks.front().data.back() = expected.nodes.max_node_tag;
    expected.elements.max_element_tag = std::numeric_limits<std::uint64_t>::max();
    expected.elements.entity_blocks.front().data[5] = expected.elements.max_element_tag;
    for (std::int32_t encoding : {0, 1}) {
        std::istringstream in(serialized(expected, encoding));
        REQUIRE_NOTHROW(portability::same(load_msh(in), expected));
    }
}

TEST_CASE("Four-byte 4.1 headers fail in ASCII binary and output", "[io][portability][malformed]")
{
    REQUIRE(explicit_format_error("$MeshFormat\n4.1 0 4\n$EndMeshFormat\n"));
    std::ostringstream out;
    out << "$MeshFormat\n4.1 1 4\n";
    binary(out, std::int32_t(1));
    out << "\n$EndMeshFormat\n";
    REQUIRE(explicit_format_error(out.str()));
    for (std::int32_t encoding : {0, 1}) {
        MshSpec spec;
        spec.mesh_format.file_type = encoding;
        spec.mesh_format.data_size = 4;
        REQUIRE(explicit_save_error(spec));
    }
}

TEST_CASE("Truncated binary fields fail explicitly even in Release", "[io][portability][malformed]")
{
    const auto contents = serialized(portability::expected_fixture(), 1);
    for (const auto* section : {"Entities", "Nodes", "Elements", "NodeData", "ElementData", "ElementNodeData"}) {
        CAPTURE(section);
        const std::string marker = std::string("$") + section + "\n";
        size_t begin = contents.find(marker) + marker.size();
        const size_t end = contents.find(std::string("\n$End") + section, begin);
        REQUIRE(end != std::string::npos);
        if (std::string(section).find("Data") != std::string::npos) {
            std::istringstream header(contents.substr(begin));
            size_t count = 0;
            header >> count;
            std::string name;
            for (size_t i = 0; i < count; ++i) header >> std::quoted(name);
            header >> count;
            double real = 0;
            for (size_t i = 0; i < count; ++i) header >> real;
            header >> count;
            std::int32_t integer = 0;
            for (size_t i = 0; i < count; ++i) header >> integer;
            header.get(); // Exactly the newline before the binary entries.
            begin += static_cast<size_t>(header.tellg());
        }
        for (size_t cut = begin; cut < end; ++cut) {
            CAPTURE(cut);
            REQUIRE(explicit_format_error(contents.substr(0, cut)));
        }
    }
    const std::string marker = "$MeshFormat\n4.1 1 8\n";
    for (size_t bytes = 0; bytes < sizeof(std::int32_t); ++bytes) {
        REQUIRE(explicit_format_error(contents.substr(0, marker.size() + bytes)));
    }
}

TEST_CASE("Impossible counts and allocation products fail before allocation", "[io][portability][malformed]")
{
    REQUIRE(explicit_format_error(ascii_section("Nodes", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Elements", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Entities", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Nodes",
        "1 6148914691236517206 1 1\n3 1 1 6148914691236517206")));
    REQUIRE(explicit_format_error(ascii_section("Elements",
        "1 2049638230412172402 1 1\n3 1 5 2049638230412172402")));
    REQUIRE(explicit_format_error(ascii_section("ElementNodeData",
        "1\n\"oversized\"\n0\n3\n0\n2147483647\n1\n1 2147483647")));
    for (const auto* section : {"Nodes", "Elements", "Entities"}) {
        auto out = binary_section(section);
        binary(out, std::numeric_limits<std::uint64_t>::max());
        for (int i = 0; i < 3; ++i) binary(out, std::uint64_t(0));
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("Nodes");
        for (std::uint64_t count : {UINT64_C(1), UINT64_C(6148914691236517206), UINT64_C(1), UINT64_C(1)}) binary(out, count);
        for (std::int32_t value : {3, 1, 1}) binary(out, value);
        binary(out, UINT64_C(6148914691236517206));
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("Elements");
        for (std::uint64_t count : {UINT64_C(1), UINT64_C(2049638230412172402), UINT64_C(1), UINT64_C(1)}) binary(out, count);
        for (std::int32_t value : {3, 1, 5}) binary(out, value);
        binary(out, UINT64_C(2049638230412172402));
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("ElementNodeData");
        out << "1\n\"oversized\"\n0\n3\n0\n2147483647\n1\n";
        binary(out, std::int32_t(1));
        binary(out, std::numeric_limits<std::int32_t>::max());
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("Entities");
        for (std::uint64_t count : {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)}) binary(out, count);
        binary(out, std::int32_t(1));
        for (int i = 0; i < 3; ++i) binary(out, 0.0);
        binary(out, std::numeric_limits<std::uint64_t>::max());
        REQUIRE(explicit_format_error(out.str()));
    }
    REQUIRE(explicit_format_error(ascii_section("Entities", "1 0 0 0\n1 0 0 0 18446744073709551615")));
    REQUIRE(explicit_format_error(ascii_section("NodeData", "18446744073709551615")));
}

TEST_CASE("Signed entities and all postprocessing retain int32 boundaries", "[io][portability]")
{
    auto expected = portability::expected_fixture();
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto minimum = std::numeric_limits<std::int32_t>::min();
    expected.entities.points.front().tag = maximum;
    expected.entities.curves.front().tag = maximum;
    expected.entities.surfaces.front().tag = maximum;
    expected.entities.volumes.front().tag = maximum;
    expected.physical_groups.front().tag = maximum;
    expected.entities.points.front().physical_group_tags = {maximum, 1};
    expected.entities.curves.front().physical_group_tags = {maximum, 1};
    expected.entities.surfaces.front().physical_group_tags = {maximum, 1};
    expected.entities.volumes.front().physical_group_tags = {maximum, 1};
    expected.entities.curves.front().boundary_point_tags = {minimum, maximum, -1};
    expected.entities.surfaces.front().boundary_curve_tags = {minimum, maximum, -1};
    expected.entities.volumes.front().boundary_surface_tags = {minimum, maximum, -1};
    expected.node_data.front().entries.front().tag = maximum;
    expected.element_data.front().entries.front().tag = maximum;
    expected.element_node_data.front().entries.front().tag = maximum;
    for (std::int32_t encoding : {0, 1}) {
        std::istringstream in(serialized(expected, encoding));
        REQUIRE_NOTHROW(portability::same(load_msh(in), expected));
    }
    for (const auto* section : {"NodeData", "ElementData", "ElementNodeData"}) {
        const std::string nodes = std::string(section) == "ElementNodeData" ? " 1" : "";
        for (const auto* tag : {"2147483648", "-2147483649"}) {
            REQUIRE(explicit_format_error(ascii_section(section,
                std::string("1\n\"out-of-range\"\n0\n3\n0\n1\n1\n") + tag + nodes + " 0")));
        }
        REQUIRE(explicit_format_error(ascii_section(section,
            "1\n\"negative-count\"\n0\n3\n0\n-1\n1")));
    }
    REQUIRE(explicit_format_error(ascii_section("Entities", "1 0 0 0\n2147483648 0 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Entities", "1 0 0 0\n1 0 0 0 1 2147483648")));
    REQUIRE(explicit_format_error(ascii_section("Entities", "0 1 0 0\n1 0 0 0 1 1 1 0 1 -2147483649")));
    REQUIRE(explicit_format_error(ascii_section("ElementNodeData", "1\n\"negative-node-count\"\n0\n3\n0\n1\n1\n1 -1")));
    REQUIRE(explicit_format_error(ascii_section("Nodes", "1 1 1 1\n2 2147483648 0 1\n1\n0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Nodes", "1 1 1 1\n2 1 0 1\n18446744073709551616\n0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Nodes", "1 1 1 1\n2 1 0 1\n-1\n0 0 0")));
}

TEST_CASE("MSH 2.2 preserves representable int32 tags and rejects narrowing", "[io][portability][v22]")
{
    for (std::int32_t encoding : {0, 1}) {
        CAPTURE(encoding);
        MshSpec spec;
        spec.mesh_format.version = "2.2";
        spec.mesh_format.file_type = encoding;
        const auto tag = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
        spec.nodes.num_entity_blocks = 1;
        spec.nodes.num_nodes = 1;
        spec.nodes.min_node_tag = tag;
        spec.nodes.max_node_tag = tag;
        spec.nodes.entity_blocks = {{0, 1, 0, 1, {tag}, {0, 0, 0}}};
        spec.elements.num_entity_blocks = 1;
        spec.elements.num_elements = 1;
        spec.elements.min_element_tag = tag;
        spec.elements.max_element_tag = tag;
        spec.elements.entity_blocks = {{0, 1, 15, 1, {tag, tag}}};
        std::ostringstream out;
        save_msh(out, spec);
        std::istringstream in(out.str());
        const auto actual = load_msh(in);
        REQUIRE(actual.nodes.entity_blocks.front().tags == spec.nodes.entity_blocks.front().tags);
        REQUIRE(actual.elements.entity_blocks.front().data == spec.elements.entity_blocks.front().data);
        auto invalid = spec;
        invalid.nodes.entity_blocks.front().tags.front() = tag + 1;
        invalid.nodes.max_node_tag = tag + 1;
        REQUIRE(explicit_save_error(invalid));
        invalid = spec;
        invalid.elements.entity_blocks.front().data.front() = tag + 1;
        invalid.elements.max_element_tag = tag + 1;
        REQUIRE(explicit_save_error(invalid));
        invalid = spec;
        invalid.elements.entity_blocks.front().data.back() = tag + 1;
        REQUIRE(explicit_save_error(invalid));
        invalid = spec;
        invalid.nodes.num_nodes = tag + 1;
        REQUIRE(explicit_save_error(invalid));
        invalid = spec;
        invalid.elements.num_elements = tag + 1;
        REQUIRE(explicit_save_error(invalid));
    }
}

TEST_CASE("Binary MSH 4.1 accepts trailing MeshFormat whitespace", "[io][portability][binary]")
{
    std::ostringstream out;
    out << "$MeshFormat\n4.1 1 8 \t \n";
    binary(out, std::int32_t(1));
    out << "\n$EndMeshFormat\n";

    std::istringstream in(out.str());
    const auto spec = load_msh(in);
    REQUIRE(spec.mesh_format.version == "4.1");
    REQUIRE(spec.mesh_format.file_type == 1);
}

TEST_CASE("Lone CR before binary MSH 2.2 node data preserves the first tag byte",
    "[io][portability][v22][binary]")
{
    std::ostringstream out;
    out << "$MeshFormat\n2.2 1 8\n";
    binary(out, std::int32_t(1));
    out << "\n$EndMeshFormat\n$Nodes\n1\r";
    binary(out, std::int32_t(10));
    binary(out, 1.0);
    binary(out, 2.0);
    binary(out, 3.0);
    out << "\n$EndNodes\n";

    std::istringstream in(out.str());
    const auto spec = load_msh(in);
    const auto& node = spec.nodes.entity_blocks.front();
    REQUIRE(spec.nodes.num_nodes == 1);
    REQUIRE(node.tags.front() == 10);
    REQUIRE(node.data[0] == 1.0);
    REQUIRE(node.data[1] == 2.0);
    REQUIRE(node.data[2] == 3.0);
}

TEST_CASE("save_msh reports a failure deferred until stream synchronization", "[io][portability]")
{
    MshSpec spec;
    spec.nodes.num_entity_blocks = 1;
    spec.nodes.num_nodes = 1;
    spec.nodes.min_node_tag = 1;
    spec.nodes.max_node_tag = 1;
    spec.nodes.entity_blocks = {{0, 1, 0, 1, {1}, {0.0, 0.0, 0.0}}};

    FailOnFourthSync buffer;
    std::ostream out(&buffer);
    REQUIRE_THROWS_AS(save_msh(out, spec), InvalidFormat);
}
