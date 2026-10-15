#include <catch2/catch_test_macros.hpp>

#include <mshio/exception.h>
#include <mshio/mshio.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace mshio;

template <typename T>
void append_binary(std::ostream& out, T value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

std::string mesh_header(const std::string& version, bool binary)
{
    std::ostringstream out;
    out << "$MeshFormat\n" << version << ' ' << (binary ? 1 : 0) << " 8\n";
    if (binary) {
        append_binary(out, std::int32_t(1));
        out << '\n';
    }
    out << "$EndMeshFormat\n";
    return out.str();
}

MshSpec parse(const std::string& text)
{
    std::istringstream in(text);
    return load_msh(in);
}

MshSpec fixture()
{
    MshSpec spec;
    CurveParametrization curve;
    curve.tag = 12;
    curve.num_nodes = 2;
    curve.data = {0., 0., 0., -1., 2., 0., 0., 3.};
    spec.parametrizations.curves.push_back(curve);
    SurfaceParametrization surface;
    surface.tag = 31;
    surface.num_nodes = 3;
    surface.num_triangles = 1;
    surface.data = {0., 0., 0.,  0.,  0.,  1., 2., 3., 4., 5., 6.,  2.,  0.,  0.,  1.,  0., 7.,
                    8., 9., 10., 11., 12., 0., 2., 0., 0., 1., 13., 14., 15., 16., 17., 18.};
    surface.triangle_indices = {0, 2, 1};
    spec.parametrizations.surfaces.push_back(surface);
    InterpolationMatrix matrix;
    matrix.num_rows = 2;
    matrix.num_columns = 3;
    matrix.data = {1., 0., 0., -1., 2., 0.5};
    InterpolationTopology topology;
    topology.element_topology = 3;
    topology.matrices.push_back(matrix);
    matrix.num_rows = 3;
    matrix.num_columns = 2;
    matrix.data = {0., 0., 1., 0., 0., 1.};
    topology.matrices.push_back(matrix);
    // Do not silently truncate valid additional matrices to Gmsh's common two.
    topology.matrices.push_back(matrix);
    InterpolationScheme scheme;
    scheme.name = "basis with \\literal\\ paths";
    scheme.topologies.push_back(topology);
    spec.interpolation_schemes.push_back(scheme);
    scheme.name = "second basis";
    scheme.topologies[0].element_topology = 10;
    spec.interpolation_schemes.push_back(scheme);
    Data data;
    data.header.string_tags = {"data", spec.interpolation_schemes[0].name};
    data.header.real_tags = {0.25};
    data.header.int_tags = {0, 1, 1};
    DataEntry entry;
    entry.tag = 93;
    entry.num_nodes_per_element = 2;
    entry.data = {2., 7.};
    data.entries.push_back(entry);
    spec.element_node_data.push_back(data);
    return spec;
}

void require_equal(const MshSpec& a, const MshSpec& b)
{
    REQUIRE(a.parametrizations.curves.size() == b.parametrizations.curves.size());
    for (size_t i = 0; i < a.parametrizations.curves.size(); ++i) {
        const auto& x = a.parametrizations.curves[i];
        const auto& y = b.parametrizations.curves[i];
        REQUIRE(x.tag == y.tag);
        REQUIRE(x.num_nodes == y.num_nodes);
        REQUIRE(x.data == y.data);
    }
    REQUIRE(a.parametrizations.surfaces.size() == b.parametrizations.surfaces.size());
    for (size_t i = 0; i < a.parametrizations.surfaces.size(); ++i) {
        const auto& x = a.parametrizations.surfaces[i];
        const auto& y = b.parametrizations.surfaces[i];
        REQUIRE(x.tag == y.tag);
        REQUIRE(x.num_nodes == y.num_nodes);
        REQUIRE(x.num_triangles == y.num_triangles);
        REQUIRE(x.data == y.data);
        REQUIRE(x.triangle_indices == y.triangle_indices);
    }
    REQUIRE(a.interpolation_schemes.size() == b.interpolation_schemes.size());
    for (size_t i = 0; i < a.interpolation_schemes.size(); ++i) {
        const auto& x = a.interpolation_schemes[i];
        const auto& y = b.interpolation_schemes[i];
        REQUIRE(x.name == y.name);
        REQUIRE(x.topologies.size() == y.topologies.size());
        for (size_t j = 0; j < x.topologies.size(); ++j) {
            REQUIRE(x.topologies[j].element_topology == y.topologies[j].element_topology);
            REQUIRE(x.topologies[j].matrices.size() == y.topologies[j].matrices.size());
            for (size_t k = 0; k < x.topologies[j].matrices.size(); ++k) {
                const auto& m = x.topologies[j].matrices[k];
                const auto& n = y.topologies[j].matrices[k];
                REQUIRE(m.num_rows == n.num_rows);
                REQUIRE(m.num_columns == n.num_columns);
                REQUIRE(m.data == n.data);
            }
        }
    }
}
} // namespace

TEST_CASE("Extra sections preserve full records without mesh data", "[extra_sections]")
{
    for (bool binary : {false, true}) {
        for (const std::string version : {"4.1", "2.2"}) {
            auto spec = fixture();
            if (version == "2.2") spec.parametrizations = {};
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = binary ? 1 : 0;
            std::ostringstream out;
            save_msh(out, spec);
            const auto text = out.str();
            REQUIRE(text.find("$InterpolationScheme") < text.find("$ElementNodeData"));
            REQUIRE(
                text.find("\"second basis\"\n1\n10\n3\n2 3\n1 0 0\n-1 2 0.5\n") !=
                std::string::npos);
            const auto loaded = parse(text);
            require_equal(spec, loaded);
            REQUIRE(loaded.nodes.entity_blocks.empty());
            REQUIRE(loaded.elements.entity_blocks.empty());
            REQUIRE(loaded.element_node_data.size() == 1);
            REQUIRE(
                loaded.element_node_data[0].header.string_tags ==
                spec.element_node_data[0].header.string_tags);
            REQUIRE(loaded.element_node_data[0].entries[0].tag == 93);
            REQUIRE(loaded.element_node_data[0].entries[0].num_nodes_per_element == 2);
            REQUIRE(loaded.element_node_data[0].entries[0].data == std::vector<double>{2., 7.});
        }
    }
}

TEST_CASE("Parametrizations read independent fixed width wire records", "[extra_sections]")
{
    const auto expected = fixture();
    for (bool binary : {false, true}) {
        std::ostringstream wire;
        wire << mesh_header("4.1", binary) << "$Parametrizations\n";
        const auto& curve = expected.parametrizations.curves[0];
        const auto& surface = expected.parametrizations.surfaces[0];
        if (binary) {
            append_binary(wire, std::uint64_t(1));
            append_binary(wire, std::uint64_t(1));
            append_binary(wire, curve.tag);
            append_binary(wire, curve.num_nodes);
            for (auto value : curve.data) append_binary(wire, value);
            append_binary(wire, surface.tag);
            append_binary(wire, surface.num_nodes);
            append_binary(wire, surface.num_triangles);
            for (auto value : surface.data) append_binary(wire, value);
            for (auto value : surface.triangle_indices) append_binary(wire, value);
            wire << '\n';
        } else {
            wire << "1 1\n12 2\n0 0 0 -1\n2 0 0 3\n31 3 1\n"
                    "0 0 0 0 0 1 2 3 4 5 6\n"
                    "2 0 0 1 0 7 8 9 10 11 12\n"
                    "0 2 0 0 1 13 14 15 16 17 18\n0 2 1\n";
        }
        wire << "$EndParametrizations\n";
        auto comparison = expected;
        comparison.interpolation_schemes.clear();
        comparison.element_node_data.clear();
        require_equal(comparison, parse(wire.str()));
    }
}

TEST_CASE("Extra section validators reject invalid local shapes", "[extra_sections]")
{
    auto original = fixture();
    auto reject = [](const MshSpec& spec) {
        std::ostringstream out;
        REQUIRE_THROWS_AS(save_msh(out, spec), InvalidFormat);
        REQUIRE_THROWS_AS(validate_spec(spec), InvalidFormat);
    };
    auto bad = original;
    bad.parametrizations.curves[0].num_nodes = 3;
    reject(bad);
    bad = original;
    bad.parametrizations.surfaces[0].data.pop_back();
    reject(bad);
    bad = original;
    bad.parametrizations.surfaces[0].num_triangles = 2;
    reject(bad);
    bad = original;
    bad.parametrizations.surfaces[0].triangle_indices[0] = -1;
    reject(bad);
    bad.parametrizations.surfaces[0].triangle_indices[0] = 3;
    reject(bad);
    bad = original;
    bad.interpolation_schemes[0].topologies[0].matrices[0].data.pop_back();
    reject(bad);
    bad = original;
    bad.interpolation_schemes[0].topologies[0].matrices[0].num_rows = -1;
    reject(bad);
    bad = original;
    bad.interpolation_schemes[0].name = "bad\"name";
    reject(bad);
    bad = original;
    bad.interpolation_schemes[0].topologies[0].element_topology = 11;
    reject(bad);
    bad = original;
    bad.mesh_format.version = "2.2";
    reject(bad);
}

TEST_CASE("Malformed extra section counts and boundaries are rejected", "[extra_sections]")
{
    const std::vector<std::string> parametrizations = {
        "1 0\n12 18446744073709551615\n",
        "1 0\n12 2\n0 0 0 0\n",
        "0 1\n31 1 1\n0 0 0 0 0 0 0 0 0 0 0\n0 1 0\n",
        "0 0\n12 0\n",
        "-1 0\n"};
    for (const auto& body : parametrizations)
        REQUIRE_THROWS_AS(
            parse(
                mesh_header("4.1", false) + "$Parametrizations\n" + body +
                "$EndParametrizations\n"),
            InvalidFormat);
    const std::vector<std::string> schemes = {
        "\"x\"\n-1\n",
        "\"x\"\n1\n3 -1\n",
        "\"x\"\n1\n3 1\n-1 2\n",
        "\"x\"\n1\n3 1\n2147483647 2147483647\n",
        "\"x\"\n1\n3 1\n2 2\n1 2 3\n",
        "\"x\"\n1\n3 1\n1 1\n1 2\n",
        "\"x\"\n0\n3 0\n"};
    for (const auto& version : {"2.2", "4.1"})
        for (bool binary : {false, true})
            for (const auto& body : schemes)
                REQUIRE_THROWS_AS(
                    parse(
                        mesh_header(version, binary) + "$InterpolationScheme\n" + body +
                        "$EndInterpolationScheme\n"),
                    InvalidFormat);
    std::ostringstream truncated;
    truncated << mesh_header("4.1", true) << "$Parametrizations\n";
    append_binary(truncated, std::uint64_t(1));
    append_binary(truncated, std::uint64_t(0));
    append_binary(truncated, std::int32_t(12));
    append_binary(truncated, std::uint64_t(2));
    append_binary(truncated, 0.0);
    REQUIRE_THROWS_AS(parse(truncated.str()), InvalidFormat);
}

TEST_CASE("Empty interpolation matrices and all documented topologies survive", "[extra_sections]")
{
    MshSpec spec;
    InterpolationScheme scheme;
    scheme.name = "empty matrices";
    for (std::int32_t id = 1; id <= 10; ++id) {
        InterpolationTopology topology;
        topology.element_topology = id;
        InterpolationMatrix matrix;
        matrix.num_rows = 2;
        topology.matrices.push_back(matrix);
        scheme.topologies.push_back(topology);
    }
    spec.interpolation_schemes.push_back(scheme);
    std::ostringstream out;
    save_msh(out, spec);
    require_equal(spec, parse(out.str()));
}
