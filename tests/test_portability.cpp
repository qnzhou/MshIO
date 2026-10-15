#include <catch2/catch_test_macros.hpp>

#include <mshio/exception.h>
#include "../src/load_msh_data.h"
#include "../src/load_msh_elements.h"
#include "../src/load_msh_entities.h"
#include "../src/load_msh_format.h"
#include "../src/load_msh_nodes.h"
#include "portability_fixture.h"

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

static_assert(
    std::is_same<decltype(NodeBlock::tags)::value_type, std::uint64_t>::value,
    "Node identifiers are format uint64_t, not host size_t");
static_assert(
    std::is_same<decltype(ElementBlock::data)::value_type, std::uint64_t>::value,
    "Element identifiers and connectivity are format uint64_t");
static_assert(
    std::is_same<decltype(Nodes::num_nodes), std::uint64_t>::value,
    "Format counts are uint64_t");
static_assert(
    std::is_same<decltype(DataEntry::tag), std::int32_t>::value,
    "Postprocessing identifiers remain specification int32_t");
static_assert(
    std::is_same<decltype(CurveEntity::boundary_point_tags)::value_type, std::int32_t>::value,
    "Entity boundaries retain their signed 32-bit width");

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
    return "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n$" + section + "\n" + payload + "\n$End" +
           section + "\n";
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
    const std::string fixture(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    const auto output = serialized(portability::expected_fixture(), 1);
    for (const auto* section :
         {"Entities", "Nodes", "Elements", "NodeData", "ElementData", "ElementNodeData"}) {
        CAPTURE(section);
        REQUIRE(binary_payload(output, section) == binary_payload(fixture, section));
    }
    const auto sparse = portability::expected_fixture(true);
    std::ostringstream expected_elements;
    for (const std::uint64_t count :
         {UINT64_C(1), UINT64_C(2), UINT64_C(13), portability::sparse_element_tag}) {
        binary(expected_elements, count);
    }
    for (const std::int32_t value : {3, 1, 4}) binary(expected_elements, value);
    binary(expected_elements, UINT64_C(2));
    for (const auto value : sparse.elements.entity_blocks.front().data)
        binary(expected_elements, value);
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
    using Reader = void (*)(std::istream&, MshSpec&);
    struct Section
    {
        const char* name;
        Reader read;
    };
    const Section sections[] = {
        {"Entities", load_entities},
        {"Nodes", load_nodes},
        {"Elements", load_elements},
        {"NodeData", load_node_data},
        {"ElementData", load_element_data},
        {"ElementNodeData", load_element_node_data},
    };
    const auto contents = serialized(portability::expected_fixture(), 1);
    for (const auto& section : sections) {
        CAPTURE(section.name);
        const std::string marker = std::string("$") + section.name + "\n";
        const size_t begin = contents.find(marker) + marker.size();
        const size_t end = contents.find(std::string("\n$End") + section.name, begin);
        REQUIRE(end != std::string::npos);
        for (size_t cut = begin; cut < end; ++cut) {
            CAPTURE(cut);
            MshSpec spec;
            spec.mesh_format.file_type = 1;
            // Invoke the section reader without dispatcher's independent terminator check.
            std::istringstream in("\n" + contents.substr(begin, cut - begin));
            REQUIRE_THROWS_AS(section.read(in, spec), InvalidFormat);
        }
    }
    const std::string marker = "$MeshFormat\n4.1 1 8\n";
    for (size_t bytes = 0; bytes < sizeof(std::int32_t); ++bytes) {
        MshSpec spec;
        std::istringstream in("4.1 1 8\n" + contents.substr(marker.size(), bytes));
        REQUIRE_THROWS_AS(load_mesh_format(in, spec), InvalidFormat);
    }
}

TEST_CASE(
    "Impossible counts and allocation products fail before allocation",
    "[io][portability][malformed]")
{
    REQUIRE(explicit_format_error(ascii_section("Nodes", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Elements", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Entities", "18446744073709551615 0 0 0")));
    REQUIRE(explicit_format_error(
        ascii_section("Nodes", "1 6148914691236517206 1 1\n3 1 1 6148914691236517206")));
    REQUIRE(explicit_format_error(
        ascii_section("Elements", "1 2049638230412172402 1 1\n3 1 5 2049638230412172402")));
    REQUIRE(explicit_format_error(ascii_section(
        "ElementNodeData",
        "1\n\"oversized\"\n0\n3\n0\n2147483647\n1\n1 2147483647")));
    for (const auto* section : {"Nodes", "Elements", "Entities"}) {
        auto out = binary_section(section);
        binary(out, std::numeric_limits<std::uint64_t>::max());
        for (int i = 0; i < 3; ++i) binary(out, std::uint64_t(0));
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("Nodes");
        for (std::uint64_t count :
             {UINT64_C(1), UINT64_C(6148914691236517206), UINT64_C(1), UINT64_C(1)})
            binary(out, count);
        for (std::int32_t value : {3, 1, 1}) binary(out, value);
        binary(out, UINT64_C(6148914691236517206));
        REQUIRE(explicit_format_error(out.str()));
    }
    {
        auto out = binary_section("Elements");
        for (std::uint64_t count :
             {UINT64_C(1), UINT64_C(2049638230412172402), UINT64_C(1), UINT64_C(1)})
            binary(out, count);
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
        for (std::uint64_t count : {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)})
            binary(out, count);
        binary(out, std::int32_t(1));
        for (int i = 0; i < 3; ++i) binary(out, 0.0);
        binary(out, std::numeric_limits<std::uint64_t>::max());
        REQUIRE(explicit_format_error(out.str()));
    }
    REQUIRE(
        explicit_format_error(ascii_section("Entities", "1 0 0 0\n1 0 0 0 18446744073709551615")));
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
            REQUIRE(explicit_format_error(ascii_section(
                section,
                std::string("1\n\"out-of-range\"\n0\n3\n0\n1\n1\n") + tag + nodes + " 0")));
        }
        REQUIRE(
            explicit_format_error(ascii_section(section, "1\n\"negative-count\"\n0\n3\n0\n-1\n1")));
    }
    REQUIRE(explicit_format_error(ascii_section("Entities", "1 0 0 0\n2147483648 0 0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Entities", "1 0 0 0\n1 0 0 0 1 2147483648")));
    REQUIRE(
        explicit_format_error(ascii_section("Entities", "0 1 0 0\n1 0 0 0 1 1 1 0 1 -2147483649")));
    REQUIRE(explicit_format_error(
        ascii_section("ElementNodeData", "1\n\"negative-node-count\"\n0\n3\n0\n1\n1\n1 -1")));
    REQUIRE(explicit_format_error(ascii_section("Nodes", "1 1 1 1\n2 2147483648 0 1\n1\n0 0 0")));
    REQUIRE(explicit_format_error(
        ascii_section("Nodes", "1 1 1 1\n2 1 0 1\n18446744073709551616\n0 0 0")));
    REQUIRE(explicit_format_error(ascii_section("Nodes", "1 1 1 1\n2 1 0 1\n-1\n0 0 0")));
}

TEST_CASE(
    "MSH 2.2 preserves representable int32 tags and rejects narrowing",
    "[io][portability][v22]")
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
        REQUIRE(
            actual.elements.entity_blocks.front().data == spec.elements.entity_blocks.front().data);
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
    }
}

TEST_CASE("MSH 2.2 rejects oversized counts before reading records", "[io][portability][v22]")
{
    for (std::int32_t encoding : {0, 1}) {
        for (const auto reader : {load_nodes, load_elements}) {
            MshSpec spec;
            spec.mesh_format.version = "2.2";
            spec.mesh_format.file_type = encoding;
            std::istringstream in("2147483648\n12345\n");
            REQUIRE_THROWS_AS(reader(in, spec), InvalidFormat);
            std::uint64_t next = 0;
            in >> next;
            REQUIRE(next == 12345);
        }
    }
}

TEST_CASE("MSH 2.2 roundtrips meshes with empty element blocks", "[io][portability][v22]")
{
    MshSpec spec;
    spec.mesh_format.version = "2.2";
    spec.nodes = {1, 1, 1, 1, {{0, 1, 0, 1, {1}, {0, 0, 0}}}};
    spec.elements = {3, 1, 1, 1, {{0, 1, 15, 0, {}}, {0, 1, 15, 1, {1, 1}}, {0, 1, 15, 0, {}}}};
    validate_spec(spec);
    for (std::int32_t encoding : {0, 1}) {
        CAPTURE(encoding);
        std::istringstream in(serialized(spec, encoding));
        const auto actual = load_msh(in);
        validate_spec(actual);
        REQUIRE(actual.elements.num_elements == 1);
        REQUIRE(actual.elements.entity_blocks.front().data == spec.elements.entity_blocks[1].data);
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

TEST_CASE(
    "Lone CR before binary MSH 2.2 node data preserves the first tag byte",
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

TEST_CASE(
    "MSH ASCII doubles match Gmsh precision without changing stream formatting",
    "[io][portability][precision]")
{
    const double value = 1.2345678901234567;
    const double small = 1.2345678901234567e-120;
    const double large = 1.2345678901234567e120;
    const double time = 0.12345678901234567;
    const std::vector<double> original = {value, small, large};
    const std::vector<double> rounded = {
        1.234567890123457,
        1.234567890123457e-120,
        1.234567890123457e120};
    MshSpec spec;
    spec.nodes = {
        1,
        2,
        1,
        2,
        {{1, 1, 1, 2, {1, 2}, {value, small, large, value, 2, 0, 0, 1.0000000000000002}}}};
    spec.elements = {1, 1, 1, 1, {{1, 1, 1, 1, {1, 1, 2}}}};
    spec.entities.points = {{1, value, small, large, {}}};
    spec.entities.curves = {{1, small, 0, 0, large, value, 0, {}, {}}};
    Data data;
    data.header = {{"precision"}, {time}, {0, 3, 1}};
    data.entries = {{1, 0, original}};
    spec.node_data = {data};
    spec.element_data = {data};
    data.entries[0].num_nodes_per_element = 2;
    data.entries[0].data.insert(data.entries[0].data.end(), original.begin(), original.end());
    spec.element_node_data = {data};
    validate_spec(spec);

    for (const auto* version : {"2.2", "4.1"}) {
        for (std::int32_t encoding : {0, 1}) {
            CAPTURE(version, encoding);
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = encoding;
            for (const auto format : {std::ios_base::fixed, std::ios_base::scientific}) {
                std::ostringstream out;
                out.setf(format, std::ios_base::floatfield);
                out.precision(3);
                const auto flags = out.flags();
                save_msh(out, spec);
                REQUIRE(out.precision() == 3);
                REQUIRE(out.flags() == flags);
                std::istringstream in(out.str());
                const auto loaded = load_msh(in);
                validate_spec(loaded);
                const auto& expected = encoding == 0 ? rounded : original;
                const auto& coordinates = loaded.nodes.entity_blocks.front().data;
                REQUIRE(coordinates[0] == expected[0]);
                REQUIRE(coordinates[1] == expected[1]);
                REQUIRE(coordinates[2] == expected[2]);
                if (spec.mesh_format.version == "4.1") {
                    REQUIRE(coordinates[3] == expected[0]);
                    REQUIRE(coordinates[7] == (encoding == 0 ? 1.0 : 1.0000000000000002));
                    REQUIRE(loaded.entities.points[0].x == expected[0]);
                    REQUIRE(loaded.entities.points[0].y == expected[1]);
                    REQUIRE(loaded.entities.points[0].z == expected[2]);
                    REQUIRE(loaded.entities.curves[0].min_x == expected[1]);
                    REQUIRE(loaded.entities.curves[0].max_x == expected[2]);
                }
                for (const auto* dataset :
                     {&loaded.node_data[0],
                      &loaded.element_data[0],
                      &loaded.element_node_data[0]}) {
                    // Dataset headers stay ASCII even in binary MSH files.
                    REQUIRE(dataset->header.real_tags[0] == 0.1234567890123457);
                    const auto& values = dataset->entries[0].data;
                    for (size_t i = 0; i < values.size(); ++i)
                        REQUIRE(values[i] == expected[i % expected.size()]);
                }
            }
        }
    }
}

TEST_CASE("Failed MSH writes restore caller floating-point formatting", "[io][precision]")
{
    MshSpec spec;
    spec.nodes = {1, 1, 1, 1, {{0, 1, 0, 1, {1}, {}}}};
    std::ostringstream out;
    out << std::scientific << std::setprecision(9);
    const auto flags = out.flags();
    REQUIRE_THROWS_AS(save_msh(out, spec), InvalidFormat);
    REQUIRE(out.precision() == 9);
    REQUIRE(out.flags() == flags);
}
