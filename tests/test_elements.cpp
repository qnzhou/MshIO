#include <catch2/catch_test_macros.hpp>

#include <mshio/exception.h>
#include <mshio/mshio.h>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct ExpectedElement
{
    std::int32_t type;
    size_t nodes;
    std::int32_t dim;
};

std::vector<ExpectedElement> expected_elements()
{
    // Independent family/order formulas from the released Gmsh 4.15.2 registry,
    // rather than a copy of the implementation's type-indexed lookup table.
    std::vector<ExpectedElement> result;
    auto add_family = [&](std::initializer_list<std::int32_t> types,
                          size_t first_order,
                          std::int32_t dim,
                          size_t family,
                          bool incomplete) {
        size_t order = first_order;
        for (auto type : types) {
            size_t nodes = 0;
            switch (family) {
            case 1: nodes = order + 1; break;
            case 2: nodes = incomplete ? 3 * order : (order + 1) * (order + 2) / 2; break;
            case 3: nodes = incomplete ? 4 * order : (order + 1) * (order + 1); break;
            case 4:
                nodes =
                    incomplete ? 4 + 6 * (order - 1) : (order + 1) * (order + 2) * (order + 3) / 6;
                break;
            case 5:
                nodes = incomplete ? 8 + 12 * (order - 1) : (order + 1) * (order + 1) * (order + 1);
                break;
            case 6:
                nodes =
                    incomplete ? 6 + 9 * (order - 1) : (order + 1) * (order + 1) * (order + 2) / 2;
                break;
            case 7:
                nodes = incomplete ? 5 + 8 * (order - 1)
                                   : (order + 1) * (order + 2) * (2 * order + 3) / 6;
                break;
            }
            result.push_back({type, nodes, dim});
            ++order;
        }
    };
    add_family({1, 8, 26, 27, 28, 62, 63, 64, 65, 66}, 1, 1, 1, false);
    add_family({2, 9, 21, 23, 25, 42, 43, 44, 45, 46}, 1, 2, 2, false);
    add_family({20, 22, 24, 52, 53, 54, 55, 56}, 3, 2, 2, true);
    add_family({3, 10, 36, 37, 38, 47, 48, 49, 50, 51}, 1, 2, 3, false);
    add_family({16, 39, 40, 41, 57, 58, 59, 60, 61}, 2, 2, 3, true);
    add_family({4, 11, 29, 30, 31, 71, 72, 73, 74, 75}, 1, 3, 4, false);
    add_family({137, 32, 33, 79, 80, 81, 82, 83}, 3, 3, 4, true);
    add_family({5, 12, 92, 93, 94, 95, 96, 97, 98}, 1, 3, 5, false);
    add_family({17, 99, 100, 101, 102, 103, 104, 105}, 2, 3, 5, true);
    add_family({6, 13, 90, 91, 106, 107, 108, 109, 110}, 1, 3, 6, false);
    add_family({18, 111, 112, 113, 114, 115, 116, 117}, 2, 3, 6, true);
    add_family({7, 14, 118, 119, 120, 121, 122, 123, 124}, 1, 3, 7, false);
    add_family({19, 125, 126, 127, 128, 129, 130, 131}, 2, 3, 7, true);
    const ExpectedElement special[] = {
        {15, 1, 0},
        {67, 2, 1},
        {68, 3, 2},
        {70, 2, 1},
        {84, 1, 1},
        {85, 1, 2},
        {86, 1, 2},
        {87, 1, 3},
        {88, 1, 3},
        {89, 1, 3},
        {132, 1, 3},
        {133, 1, 0},
        {134, 2, 1},
        {135, 3, 2},
        {136, 4, 3},
        {140, 4, 3}};
    result.insert(result.end(), std::begin(special), std::end(special));
    std::sort(result.begin(), result.end(), [](const ExpectedElement& a, const ExpectedElement& b) {
        return a.type < b.type;
    });
    return result;
}

mshio::MshSpec element_fixture(const std::vector<ExpectedElement>& types)
{
    mshio::MshSpec spec;
    auto& nodes = spec.nodes;
    nodes.num_entity_blocks = 1;
    nodes.num_nodes = nodes.max_node_tag = 1000;
    nodes.min_node_tag = 1;
    nodes.entity_blocks.emplace_back();
    auto& node_block = nodes.entity_blocks.back();
    node_block.entity_dim = 3;
    node_block.entity_tag = 1;
    node_block.num_nodes_in_block = 1000;
    for (std::uint64_t tag = 1; tag <= 1000; ++tag) {
        node_block.tags.push_back(tag);
        node_block.data.insert(
            node_block.data.end(),
            {static_cast<double>(tag), static_cast<double>(tag % 17), 0.25});
    }
    auto& elements = spec.elements;
    elements.num_entity_blocks = static_cast<std::uint64_t>(types.size());
    elements.num_elements = elements.max_element_tag = static_cast<std::uint64_t>(types.size()) * 2;
    elements.min_element_tag = 1;
    std::uint64_t tag = 1;
    for (const auto& type : types) {
        mshio::ElementBlock block;
        block.entity_dim = type.dim;
        block.entity_tag = type.type;
        block.element_type = type.type;
        block.num_elements_in_block = 2;
        for (size_t record = 0; record < 2; ++record) {
            block.data.push_back(tag++);
            for (size_t node = 0; node < type.nodes; ++node)
                block.data.push_back(
                    static_cast<std::uint64_t>((node + record * 17) % type.nodes + 1));
        }
        elements.entity_blocks.push_back(std::move(block));
    }
    return spec;
}

template <typename T>
void binary_value(std::ostream& out, T value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

std::string element_header(const char* version, int binary, std::int32_t type, std::int32_t dim = 3)
{
    // A deliberately incomplete record is enough: unsupported types/dimensions
    // must be rejected before the reader sizes or consumes connectivity.
    std::ostringstream out;
    out << "$MeshFormat\n" << version << ' ' << binary << " 8\n";
    if (binary) {
        binary_value(out, std::int32_t(1));
        out << '\n';
    }
    out << "$EndMeshFormat\n$Elements\n";
    if (std::string(version) == "4.1") {
        if (binary) {
            for (int i = 0; i < 4; ++i) binary_value(out, std::uint64_t(1));
            binary_value(out, dim);
            binary_value(out, std::int32_t(1));
            binary_value(out, type);
            binary_value(out, std::uint64_t(1));
        } else
            out << "1 1 1 1\n" << dim << " 1 " << type << " 1\n";
    } else {
        out << "1\n";
        if (binary) {
            binary_value(out, type);
            binary_value(out, std::int32_t(1));
            binary_value(out, std::int32_t(0));
        } else
            out << "1 " << type << " 0\n";
    }
    return out.str();
}

} // namespace

TEST_CASE("Every fixed-width released Gmsh type has exact metadata", "[elements]")
{
    const auto types = expected_elements();
    REQUIRE(types.size() == 132);
    for (const auto& type : types) {
        INFO("MSH type " << type.type);
        REQUIRE(mshio::nodes_per_element(type.type) == type.nodes);
        REQUIRE(mshio::get_element_dim(type.type) == type.dim);
    }
    for (std::int32_t type = 1; type <= 140; ++type) {
        const auto match =
            std::find_if(types.begin(), types.end(), [type](const ExpectedElement& entry) {
                return entry.type == type;
            });
        if (match == types.end()) {
            INFO("Rejected MSH type " << type);
            REQUIRE_THROWS_AS(mshio::nodes_per_element(type), mshio::UnsupportedFeature);
            REQUIRE_THROWS_AS(mshio::get_element_dim(type), mshio::UnsupportedFeature);
        }
    }
    for (auto type :
         {std::numeric_limits<std::int32_t>::min(),
          -1,
          0,
          141,
          std::numeric_limits<std::int32_t>::max()}) {
        INFO("Out-of-range MSH type " << type);
        REQUIRE_THROWS_AS(mshio::nodes_per_element(type), mshio::UnsupportedFeature);
        REQUIRE_THROWS_AS(mshio::get_element_dim(type), mshio::UnsupportedFeature);
    }
}

TEST_CASE("All fixed-width elements roundtrip in all four encodings", "[elements]")
{
    auto spec = element_fixture(expected_elements());
    for (const char* version : {"2.2", "4.1"}) {
        for (int binary : {0, 1}) {
            INFO("MSH " << version << " binary=" << binary);
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = binary;
            REQUIRE_NOTHROW(mshio::validate_spec(spec));
            std::stringstream stream;
            mshio::save_msh(stream, spec);
            const auto loaded = mshio::load_msh(stream);
            REQUIRE_NOTHROW(mshio::validate_spec(loaded));
            REQUIRE(loaded.elements.num_elements == spec.elements.num_elements);
            REQUIRE(loaded.elements.num_entity_blocks == spec.elements.num_entity_blocks);
            REQUIRE(loaded.elements.min_element_tag == spec.elements.min_element_tag);
            REQUIRE(loaded.elements.max_element_tag == spec.elements.max_element_tag);
            for (size_t i = 0; i < spec.elements.entity_blocks.size(); ++i) {
                const auto& expected = spec.elements.entity_blocks[i];
                const auto& actual = loaded.elements.entity_blocks[i];
                INFO("MSH type " << expected.element_type);
                REQUIRE(actual.element_type == expected.element_type);
                REQUIRE(actual.entity_dim == expected.entity_dim);
                REQUIRE(actual.entity_tag == expected.entity_tag);
                REQUIRE(actual.num_elements_in_block == 2);
                REQUIRE(actual.data == expected.data);
            }
        }
    }
}

TEST_CASE("Undefined and variable-width element records are rejected", "[elements]")
{
    for (const char* version : {"2.2", "4.1"}) {
        for (int binary : {0, 1}) {
            for (std::int32_t type : {0, 34, 35, 69, 76, 77, 78, 138, 139, 141}) {
                INFO("MSH " << version << " binary=" << binary << " type=" << type);
                std::istringstream input(element_header(version, binary, type));
                REQUIRE_THROWS_AS(mshio::load_msh(input), mshio::UnsupportedFeature);
                auto spec = element_fixture({{type, 1, 3}});
                spec.mesh_format.version = version;
                spec.mesh_format.file_type = binary;
                std::ostringstream output;
                REQUIRE_THROWS_AS(mshio::save_msh(output, spec), mshio::UnsupportedFeature);
            }
        }
    }
}

TEST_CASE("Expanded element records enforce counts and dimension bounds", "[elements]")
{
    for (const char* version : {"2.2", "4.1"}) {
        for (int binary : {0, 1}) {
            auto spec = element_fixture({{98, 1000, 3}});
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = binary;
            spec.elements.entity_blocks[0].data.pop_back();
            std::ostringstream output;
            REQUIRE_THROWS_AS(mshio::save_msh(output, spec), mshio::InvalidFormat);
            REQUIRE_THROWS_AS(mshio::validate_spec(spec), mshio::CorruptData);
        }
    }
    for (int binary : {0, 1}) {
        for (std::int32_t dim : {-1, 4}) {
            std::istringstream input(element_header("4.1", binary, 98, dim));
            REQUIRE_THROWS_AS(mshio::load_msh(input), mshio::InvalidFormat);
            auto spec = element_fixture({{98, 1000, dim}});
            spec.mesh_format.file_type = binary;
            std::ostringstream output;
            REQUIRE_THROWS_AS(mshio::save_msh(output, spec), mshio::InvalidFormat);
            REQUIRE_THROWS_AS(mshio::validate_spec(spec), mshio::CorruptData);
        }
    }
}
