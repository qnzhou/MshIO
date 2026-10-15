#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <sstream>
#include <utility>

#include <mshio/exception.h>
#include <mshio/mshio.h>

namespace {

using namespace mshio;

void ASSERT_SAME_NODES(const MshSpec& spec1, const MshSpec& spec2)
{
    const auto& nodes1 = spec1.nodes;
    const auto& nodes2 = spec2.nodes;

    REQUIRE(nodes1.num_entity_blocks == nodes2.num_entity_blocks);
    REQUIRE(nodes1.num_nodes == nodes2.num_nodes);
    REQUIRE(nodes1.min_node_tag == nodes2.min_node_tag);
    REQUIRE(nodes1.max_node_tag == nodes2.max_node_tag);

    const size_t N = nodes1.entity_blocks.size();
    for (size_t i = 0; i < N; i++) {
        const auto& block1 = nodes1.entity_blocks[i];
        const auto& block2 = nodes2.entity_blocks[i];

        REQUIRE(block1.entity_dim == block2.entity_dim);
        REQUIRE(block1.entity_tag == block2.entity_tag);
        REQUIRE(block1.parametric == block2.parametric);
        REQUIRE(block1.tags == block2.tags);
        REQUIRE(block1.data == block2.data);
    }
}

void ASSERT_SAME_ELEMENTS(const MshSpec& spec1, const MshSpec& spec2)
{
    const auto& elements1 = spec1.elements;
    const auto& elements2 = spec2.elements;

    REQUIRE(elements1.num_entity_blocks == elements2.num_entity_blocks);
    REQUIRE(elements1.num_elements == elements2.num_elements);
    REQUIRE(elements1.min_element_tag == elements2.min_element_tag);
    REQUIRE(elements1.max_element_tag == elements2.max_element_tag);

    const size_t M = elements1.entity_blocks.size();
    for (size_t i = 0; i < M; i++) {
        const auto& block1 = elements1.entity_blocks[i];
        const auto& block2 = elements2.entity_blocks[i];

        REQUIRE(block1.entity_dim == block2.entity_dim);
        REQUIRE(block1.entity_tag == block2.entity_tag);
        REQUIRE(block1.element_type == block2.element_type);
        REQUIRE(block1.num_elements_in_block == block2.num_elements_in_block);
        REQUIRE(block1.data == block2.data);
    }
}

void ASSERT_SAME_NODE_DATA(const MshSpec& spec1, const MshSpec& spec2)
{
    const auto& node_data1 = spec1.node_data;
    const auto& node_data2 = spec2.node_data;
    REQUIRE(node_data1.size() == node_data2.size());
    const size_t K = node_data1.size();

    for (size_t i = 0; i < K; i++) {
        const auto& data1 = node_data1[i];
        const auto& data2 = node_data2[i];

        REQUIRE(data1.header.string_tags == data2.header.string_tags);
        REQUIRE(data1.header.real_tags == data2.header.real_tags);
        REQUIRE(data1.header.int_tags == data2.header.int_tags);
        REQUIRE(data1.entries.size() == data2.entries.size());

        const size_t num_entries = data1.entries.size();
        for (size_t j = 0; j < num_entries; j++) {
            const auto& entry1 = data1.entries[j];
            const auto& entry2 = data2.entries[j];
            REQUIRE(entry1.tag == entry2.tag);
            REQUIRE(entry1.data == entry2.data);
        }
    }
}

void ASSERT_SAME_ELEMENT_DATA(const MshSpec& spec1, const MshSpec& spec2)
{
    const auto& elem_data1 = spec1.element_data;
    const auto& elem_data2 = spec2.element_data;
    REQUIRE(elem_data1.size() == elem_data2.size());
    const size_t K = elem_data1.size();

    for (size_t i = 0; i < K; i++) {
        const auto& data1 = elem_data1[i];
        const auto& data2 = elem_data2[i];

        REQUIRE(data1.header.string_tags == data2.header.string_tags);
        REQUIRE(data1.header.real_tags == data2.header.real_tags);
        REQUIRE(data1.header.int_tags == data2.header.int_tags);
        REQUIRE(data1.entries.size() == data2.entries.size());

        const size_t num_entries = data1.entries.size();
        for (size_t j = 0; j < num_entries; j++) {
            const auto& entry1 = data1.entries[j];
            const auto& entry2 = data2.entries[j];
            REQUIRE(entry1.tag == entry2.tag);
            REQUIRE(entry1.data == entry2.data);
        }
    }
}

void ASSERT_SAME_ELEMENT_NODE_DATA(const MshSpec& spec1, const MshSpec& spec2)
{
    const auto& elem_data1 = spec1.element_node_data;
    const auto& elem_data2 = spec2.element_node_data;
    REQUIRE(elem_data1.size() == elem_data2.size());
    const size_t K = elem_data1.size();

    for (size_t i = 0; i < K; i++) {
        const auto& data1 = elem_data1[i];
        const auto& data2 = elem_data2[i];

        REQUIRE(data1.header.string_tags == data2.header.string_tags);
        REQUIRE(data1.header.real_tags == data2.header.real_tags);
        REQUIRE(data1.header.int_tags == data2.header.int_tags);
        REQUIRE(data1.entries.size() == data2.entries.size());

        const size_t num_entries = data1.entries.size();
        for (size_t j = 0; j < num_entries; j++) {
            const auto& entry1 = data1.entries[j];
            const auto& entry2 = data2.entries[j];
            REQUIRE(entry1.tag == entry2.tag);
            REQUIRE(entry1.num_nodes_per_element == entry2.num_nodes_per_element);
            REQUIRE(entry1.data == entry2.data);
        }
    }
}

void ASSERT_SAME_CURVES(const MshSpec& spec1, const MshSpec& spec2)
{
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& curves1 = spec1.curves;
    const auto& curves2 = spec2.curves;

    REQUIRE(curves1.size() == curves2.size());

    const size_t N = curves1.size();
    for (size_t i = 0; i < N; i++) {
        const auto& c1 = curves1[i];
        const auto& c2 = curves2[i];

        REQUIRE(c1.curve_tag == c2.curve_tag);
        REQUIRE(c1.curve_type == c2.curve_type);
        REQUIRE(c1.curve_degree == c2.curve_degree);
        REQUIRE(c1.num_control_points == c2.num_control_points);
        REQUIRE(c1.num_knots == c2.num_knots);
        REQUIRE(c1.with_weights == c2.with_weights);
        REQUIRE(c1.data == c2.data);
    }
#endif
}

void ASSERT_SAME(const MshSpec& spec1, const MshSpec& spec2)
{
    ASSERT_SAME_NODES(spec1, spec2);
    ASSERT_SAME_ELEMENTS(spec1, spec2);
    ASSERT_SAME_NODE_DATA(spec1, spec2);
    ASSERT_SAME_ELEMENT_DATA(spec1, spec2);
    ASSERT_SAME_ELEMENT_NODE_DATA(spec1, spec2);

    ASSERT_SAME_CURVES(spec1, spec2);
}

void save_and_load(MshSpec& spec)
{
    std::stringstream contents;
    SECTION("v4.1")
    {
        spec.mesh_format.version = "4.1";
        SECTION("ASCII")
        {
            spec.mesh_format.file_type = 0;
        }
        SECTION("Binary")
        {
            spec.mesh_format.file_type = 1;
        }
    }
    SECTION("v2.2")
    {
        spec.mesh_format.version = "2.2";
        SECTION("ASCII")
        {
            spec.mesh_format.file_type = 0;
        }
        SECTION("Binary")
        {
            spec.mesh_format.file_type = 1;
        }
    }

    validate_spec(spec);
    save_msh(contents, spec);
    MshSpec spec2 = load_msh(contents);
    ASSERT_SAME(spec, spec2);
}

} // namespace

TEST_CASE("Load", "[io]")
{
    using namespace mshio;
    MshSpec spec;
    SECTION("v2.2 ascii")
    {
        spec = load_msh(MSHIO_DATA_DIR "/test_2.2_ascii.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v2.2 binary")
    {
        spec = load_msh(MSHIO_DATA_DIR "/test_2.2_bin.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v4.1 ascii")
    {
        spec = load_msh(MSHIO_DATA_DIR "/test_4.1_ascii.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v4.1 binary")
    {
        spec = load_msh(MSHIO_DATA_DIR "/test_4.1_bin.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }
    save_and_load(spec);
}

TEST_CASE("Empty", "[empty][io]")
{
    using namespace mshio;

    MshSpec spec;
    save_and_load(spec);
}

TEST_CASE("Point element", "[point][io]")
{
    using namespace mshio;

    MshSpec spec;
    auto& nodes = spec.nodes;

    nodes.num_entity_blocks = 1;
    nodes.num_nodes = 1;
    nodes.min_node_tag = 1;
    nodes.max_node_tag = 1;
    nodes.entity_blocks.resize(1);

    auto& node_block = nodes.entity_blocks[0];
    node_block.entity_dim = 0;
    node_block.entity_tag = 1;
    node_block.num_nodes_in_block = 1;
    node_block.tags.push_back(1);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);

    validate_spec(spec);

    auto& elements = spec.elements;
    elements.num_entity_blocks = 1;
    elements.num_elements = 1;
    elements.min_element_tag = 1;
    elements.max_element_tag = 1;
    elements.entity_blocks.resize(1);

    auto& element_block = elements.entity_blocks[0];
    element_block.entity_dim = 0;
    element_block.entity_tag = 1;
    element_block.element_type = 15;
    element_block.num_elements_in_block = 1;
    element_block.data.push_back(1); // Element tag
    element_block.data.push_back(1); // Node tag

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("Line element", "[line][io]")
{
    using namespace mshio;

    MshSpec spec;
    auto& nodes = spec.nodes;

    nodes.num_entity_blocks = 1;
    nodes.num_nodes = 2;
    nodes.min_node_tag = 1;
    nodes.max_node_tag = 2;
    nodes.entity_blocks.resize(1);

    auto& node_block = nodes.entity_blocks[0];
    node_block.entity_dim = 1;
    node_block.entity_tag = 1;
    node_block.num_nodes_in_block = 2;
    node_block.tags.push_back(1);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.tags.push_back(2);
    node_block.data.push_back(1.0);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);

    validate_spec(spec);

    auto& elements = spec.elements;
    elements.num_entity_blocks = 1;
    elements.num_elements = 1;
    elements.min_element_tag = 1;
    elements.max_element_tag = 1;
    elements.entity_blocks.resize(1);

    auto& element_block = elements.entity_blocks[0];
    element_block.entity_dim = 1;
    element_block.entity_tag = 1;
    element_block.element_type = 1;
    element_block.num_elements_in_block = 1;
    element_block.data.push_back(1); // Element tag
    element_block.data.push_back(1);
    element_block.data.push_back(2);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("triangle element", "[triangle][io]")
{
    using namespace mshio;

    MshSpec spec;
    auto& nodes = spec.nodes;

    nodes.num_entity_blocks = 1;
    nodes.num_nodes = 3;
    nodes.min_node_tag = 1;
    nodes.max_node_tag = 3;
    nodes.entity_blocks.resize(1);

    auto& node_block = nodes.entity_blocks[0];
    node_block.entity_dim = 2;
    node_block.entity_tag = 1;
    node_block.num_nodes_in_block = 3;
    node_block.tags.push_back(1);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.tags.push_back(2);
    node_block.data.push_back(1.0);
    node_block.data.push_back(0.0);
    node_block.data.push_back(0.0);
    node_block.tags.push_back(3);
    node_block.data.push_back(1.0);
    node_block.data.push_back(1.0);
    node_block.data.push_back(0.0);

    validate_spec(spec);

    auto& elements = spec.elements;
    elements.num_entity_blocks = 1;
    elements.num_elements = 2;
    elements.min_element_tag = 1;
    elements.max_element_tag = 2;
    elements.entity_blocks.resize(1);

    auto& element_block = elements.entity_blocks[0];
    element_block.entity_dim = 2;
    element_block.entity_tag = 1;
    element_block.element_type = 2;
    element_block.num_elements_in_block = 2;
    element_block.data.push_back(1); // Element tag
    element_block.data.push_back(1);
    element_block.data.push_back(2);
    element_block.data.push_back(3);
    element_block.data.push_back(2); // Element tag
    element_block.data.push_back(1);
    element_block.data.push_back(2);
    element_block.data.push_back(3);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("mixed element", "[mixed][io]")
{
    using namespace mshio;

    MshSpec spec;
    auto& nodes = spec.nodes;

    nodes.num_entity_blocks = 2;
    nodes.num_nodes = 8;
    nodes.min_node_tag = 1;
    nodes.max_node_tag = 8;
    nodes.entity_blocks.resize(2);

    auto& node_block_1 = nodes.entity_blocks[0];
    node_block_1.entity_dim = 2;
    node_block_1.entity_tag = 1;
    node_block_1.num_nodes_in_block = 4;
    node_block_1.tags.push_back(1);
    node_block_1.data.push_back(0.0);
    node_block_1.data.push_back(0.0);
    node_block_1.data.push_back(0.0);
    node_block_1.tags.push_back(2);
    node_block_1.data.push_back(1.0);
    node_block_1.data.push_back(0.0);
    node_block_1.data.push_back(0.0);
    node_block_1.tags.push_back(3);
    node_block_1.data.push_back(1.0);
    node_block_1.data.push_back(1.0);
    node_block_1.data.push_back(0.0);
    node_block_1.tags.push_back(4);
    node_block_1.data.push_back(0.0);
    node_block_1.data.push_back(1.0);
    node_block_1.data.push_back(0.0);

    auto& node_block_2 = nodes.entity_blocks[1];
    node_block_2.entity_dim = 1;
    node_block_2.entity_tag = 1;
    node_block_2.num_nodes_in_block = 4;
    node_block_2.tags.push_back(5);
    node_block_2.data.push_back(0.0);
    node_block_2.data.push_back(0.0);
    node_block_2.data.push_back(0.0);
    node_block_2.tags.push_back(6);
    node_block_2.data.push_back(1.0);
    node_block_2.data.push_back(0.0);
    node_block_2.data.push_back(0.0);
    node_block_2.tags.push_back(7);
    node_block_2.data.push_back(1.0);
    node_block_2.data.push_back(1.0);
    node_block_2.data.push_back(0.0);
    node_block_2.tags.push_back(8);
    node_block_2.data.push_back(0.0);
    node_block_2.data.push_back(1.0);
    node_block_2.data.push_back(0.0);


    validate_spec(spec);

    auto& elements = spec.elements;
    elements.num_entity_blocks = 2;
    elements.num_elements = 5;
    elements.min_element_tag = 1;
    elements.max_element_tag = 5;
    elements.entity_blocks.resize(2);

    auto& element_block_1 = elements.entity_blocks[0];
    element_block_1.entity_dim = 2;
    element_block_1.entity_tag = 1;
    element_block_1.element_type = 3;
    element_block_1.num_elements_in_block = 1;
    element_block_1.data.push_back(1); // Element tag
    element_block_1.data.push_back(1);
    element_block_1.data.push_back(2);
    element_block_1.data.push_back(3);
    element_block_1.data.push_back(4);

    auto& element_block_2 = elements.entity_blocks[1];
    element_block_2.entity_dim = 1;
    element_block_2.entity_tag = 1;
    element_block_2.element_type = 1;
    element_block_2.num_elements_in_block = 4;
    element_block_2.data.push_back(2); // Element tag
    element_block_2.data.push_back(5);
    element_block_2.data.push_back(6);
    element_block_2.data.push_back(3); // Element tag
    element_block_2.data.push_back(6);
    element_block_2.data.push_back(7);
    element_block_2.data.push_back(4); // Element tag
    element_block_2.data.push_back(7);
    element_block_2.data.push_back(8);
    element_block_2.data.push_back(5); // Element tag
    element_block_2.data.push_back(8);
    element_block_2.data.push_back(5);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("node data")
{
    using namespace mshio;

    MshSpec spec;
    spec.mesh_format.file_type = 1;
    auto& node_data = spec.node_data;

    Data attr;
    attr.header.string_tags = {"test"};
    attr.header.real_tags = {0};
    attr.header.int_tags = {0, 1, 2, 0};

    DataEntry entry1;
    entry1.tag = 1;
    entry1.data = {1};

    DataEntry entry2;
    entry2.tag = 2;
    entry2.data = {2};

    attr.entries.push_back(entry1);
    attr.entries.push_back(entry2);

    node_data.push_back(attr);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("element data")
{
    using namespace mshio;

    MshSpec spec;
    spec.mesh_format.file_type = 1;

    auto add_node_data = [&](const std::string& name, const auto& data) {
        auto& node_data = spec.node_data;

        Data attr;
        attr.header.string_tags = {name};
        attr.header.real_tags = {0};
        attr.header.int_tags = {0, 1, static_cast<int>(data.size()), 0, 3};

        std::int32_t tag = 9;
        for (const auto& value : data) {
            DataEntry entry;
            entry.tag = tag;
            entry.data = {value};
            tag++;
            attr.entries.push_back(std::move(entry));
        }
        node_data.push_back(attr);
    };

    auto add_element_data = [&](const std::string& name, const auto& data) {
        auto& elem_data = spec.element_data;

        Data attr;
        attr.header.string_tags = {name};
        attr.header.real_tags = {0};
        attr.header.int_tags = {0, 1, static_cast<int>(data.size()), 0, 10};

        std::int32_t tag = 10;
        for (const auto& value : data) {
            DataEntry entry;
            entry.tag = tag;
            entry.data = {value};
            tag++;
            attr.entries.push_back(std::move(entry));
        }

        elem_data.push_back(attr);
    };

    std::vector<double> values = {0, 1, 2, 3};
    add_node_data("tv index", values);

    std::vector<double> ids = {1.0};
    add_element_data("t index", ids);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("element node data")
{
    using namespace mshio;

    MshSpec spec;
    spec.mesh_format.file_type = 1;

    auto add_element_node_data = [&](const std::string& name, const auto& data) {
        auto& elem_node_data = spec.element_node_data;

        Data attr;
        attr.header.string_tags = {name};
        attr.header.real_tags = {0};
        attr.header.int_tags = {0, 1, static_cast<int>(data.size()), 0, 10};

        std::int32_t tag = 10;
        for (const auto& value : data) {
            DataEntry entry;
            entry.tag = tag;
            entry.num_nodes_per_element = 3;
            entry.data = {value, value, value};
            tag++;
            attr.entries.push_back(std::move(entry));
        }

        elem_node_data.push_back(attr);
    };

    std::vector<double> ids = {1.0};
    add_element_node_data("c index", ids);

    validate_spec(spec);
    save_and_load(spec);
}

TEST_CASE("physical group")
{
    using namespace mshio;

    MshSpec spec;

    const int num_vertices = 3;
    const int DIM = 2;

    {
        spec.nodes.num_entity_blocks = 1;
        spec.nodes.num_nodes = num_vertices;
        spec.nodes.min_node_tag = 1;
        spec.nodes.max_node_tag = num_vertices;

        spec.nodes.entity_blocks.resize(1);
        NodeBlock& block = spec.nodes.entity_blocks[0];
        block.num_nodes_in_block = num_vertices;
        block.tags.reserve(num_vertices);
        block.data.reserve(num_vertices * 3);
        block.entity_dim = DIM;
        block.entity_tag = 1;

        block.tags.push_back(1);
        block.data.push_back(0.0);
        block.data.push_back(0.0);
        block.data.push_back(0.0);
        block.tags.push_back(2);
        block.data.push_back(1.0);
        block.data.push_back(0.0);
        block.data.push_back(0.0);
        block.tags.push_back(3);
        block.data.push_back(1.0);
        block.data.push_back(1.0);
        block.data.push_back(0.0);
    }
    {
        spec.elements.num_entity_blocks = 1;
        spec.elements.num_elements = 1;
        spec.elements.min_element_tag = 1;
        spec.elements.max_element_tag = 1;
        spec.elements.entity_blocks.resize(1);

        auto& element_block = spec.elements.entity_blocks[0];
        element_block.entity_dim = DIM;
        element_block.entity_tag = 1;
        element_block.element_type = 2;
        element_block.num_elements_in_block = 1;
        element_block.data.push_back(1); // Element tag
        element_block.data.push_back(1);
        element_block.data.push_back(2);
        element_block.data.push_back(3);
    }
    // entities and physical groups
    {
        spec.physical_groups.resize(1);
        PhysicalGroup& ph = spec.physical_groups[0];
        ph.dim = 2;
        ph.tag = 1;
        ph.name = "my_physical_group";

        spec.entities.surfaces.resize(1);
        SurfaceEntity& e = spec.entities.surfaces[0];
        e.tag = 1;
        e.min_x = 0.0;
        e.min_y = 0.0;
        e.min_z = 0.0;
        e.max_x = 1.0;
        e.max_y = 1.0;
        e.max_z = 0.0;
        e.physical_group_tags.push_back(1);
    }

    validate_spec(spec);
    save_and_load(spec);
    //spec.mesh_format.file_type = 1; // binary
    // save_msh("test.msh", spec);
}

#ifdef MSHIO_EXT_NANOSPLINE
TEST_CASE("NanoSpline extension", "[nanospline][ext][io]")
{
    using namespace mshio;

    MshSpec spec;

    {
        spec.curves.emplace_back();
        auto& curve = spec.curves.back();
        curve.curve_tag = 1;
        curve.curve_type = 1;
        curve.curve_degree = 3;
        curve.num_control_points = 4;
        curve.num_knots = 0;
        curve.with_weights = 0;

        curve.data = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0};
    }

    {
        spec.curves.emplace_back();
        auto& curve = spec.curves.back();
        curve.curve_tag = 2;
        curve.curve_type = 2;
        curve.curve_degree = 3;
        curve.num_control_points = 4;
        curve.num_knots = 0;
        curve.with_weights = 1;

        curve.data =
            {0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0};
    }

    {
        spec.patches.emplace_back();
        auto& patch = spec.patches.back();
        patch.patch_tag = 1;
        patch.patch_type = 2;
        patch.degree_u = 3;
        patch.degree_v = 3;
        patch.num_control_points = 16;
        patch.num_u_knots = 0;
        patch.num_v_knots = 0;
        patch.with_weights = 1;

        patch.data = {
            0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 1.0, 3.0, 0.0, 0.0, 1.0,

            0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 2.0, 1.0, 0.0, 1.0, 3.0, 1.0, 0.0, 1.0,

            0.0, 2.0, 0.0, 1.0, 1.0, 2.0, 0.0, 1.0, 2.0, 2.0, 0.0, 1.0, 3.0, 2.0, 0.0, 1.0,

            0.0, 3.0, 0.0, 1.0, 1.0, 3.0, 0.0, 1.0, 2.0, 3.0, 0.0, 1.0, 3.0, 3.0, 0.0, 1.0};
    }

    validate_spec(spec);
    save_and_load(spec);
}
#endif

namespace {

// Decode the legacy wire layout independently of load_msh, so a matching
// reader/writer mistake cannot hide a physical/entity tag swap.
std::vector<std::vector<std::int32_t>> legacy_wire_tags(const std::string& contents, bool binary)
{
    const auto start = contents.find("$Elements\n");
    REQUIRE(start != std::string::npos);
    std::istringstream in(contents.substr(start + 10));
    std::uint64_t count = 0;
    in >> count;
    in.get();
    auto integer = [&]() {
        std::int32_t value = 0;
        if (binary)
            in.read(reinterpret_cast<char*>(&value), sizeof(value));
        else
            in >> value;
        REQUIRE(in.good());
        return value;
    };
    std::vector<std::vector<std::int32_t>> result;
    while (result.size() < count) {
        if (!binary) integer(); // element identifier
        const auto type = integer();
        const auto block_count = binary ? integer() : 1;
        const auto num_tags = integer();
        REQUIRE(block_count > 0);
        REQUIRE(num_tags >= 0);
        for (std::int32_t i = 0; i < block_count; ++i) {
            if (binary) integer();
            std::vector<std::int32_t> tags;
            for (std::int32_t j = 0; j < num_tags; ++j) tags.push_back(integer());
            result.push_back(std::move(tags));
            for (size_t j = 0; j < mshio::nodes_per_element(type); ++j) integer();
        }
    }
    REQUIRE(result.size() == count);
    return result;
}

} // namespace

TEST_CASE("MSH 2.2 preserves per-element physical partition and ghost tags", "[io][v22]")
{
    using namespace mshio;
    const std::vector<std::vector<std::int32_t>> input_tags =
        {{7, 9, 2, 1, -2, 42}, {7, 9, 2, 1, -2, 42}, {8, 9, 1, 3}, {0, 9}, {5}, {}};
    const std::vector<std::vector<std::int32_t>> expected =
        {{7, 9, 2, 1, -2, 42}, {7, 9, 2, 1, -2, 42}, {8, 9, 1, 3}, {0, 9}, {5, 0}, {0, 0}};
    for (std::int32_t input_encoding : {0, 1}) {
        CAPTURE(input_encoding);
        std::ostringstream source;
        auto integer = [&](std::int32_t value) {
            source.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        source << "$MeshFormat\n2.2 " << input_encoding << " 8\n";
        if (input_encoding) {
            integer(1);
            source << '\n';
        }
        source << "$EndMeshFormat\n$PhysicalNames\n2\n0 7 \"first\"\n0 8 \"second\"\n"
                  "$EndPhysicalNames\n$Nodes\n1\n";
        if (input_encoding) {
            integer(1);
            const double xyz[] = {0, 0, 0};
            source.write(reinterpret_cast<const char*>(xyz), sizeof(xyz));
            source << '\n';
        } else
            source << "1 0 0 0\n";
        source << "$EndNodes\n$Elements\n" << input_tags.size() << '\n';
        std::int32_t id = 0;
        for (const auto& tags : input_tags) {
            ++id;
            const auto num_tags = static_cast<std::int32_t>(tags.size());
            if (input_encoding) {
                integer(15);
                integer(1);
                integer(num_tags);
                integer(id);
                for (auto tag : tags) integer(tag);
                integer(1);
            } else {
                source << id << " 15 " << num_tags << ' ';
                for (auto tag : tags) source << tag << ' ';
                source << "1\n";
            }
        }
        source << "\n$EndElements\n";
        std::istringstream in(source.str());
        auto spec = load_msh(in);
        validate_spec(spec);
        REQUIRE(spec.entities.points.size() == 2);
        REQUIRE(spec.entities.points[0].tag == 0);
        REQUIRE(spec.entities.points[0].physical_group_tags == std::vector<std::int32_t>{5});
        REQUIRE(spec.entities.points[1].tag == 9);
        REQUIRE(spec.entities.points[1].physical_group_tags == std::vector<std::int32_t>{7, 8});
        REQUIRE(spec.elements.entity_blocks[0].num_elements_in_block == 2);
        for (std::int32_t output_encoding : {0, 1}) {
            CAPTURE(output_encoding);
            spec.mesh_format.file_type = output_encoding;
            std::ostringstream out;
            save_msh(out, spec);
            REQUIRE(legacy_wire_tags(out.str(), output_encoding != 0) == expected);
            std::istringstream saved(out.str());
            const auto actual = load_msh(saved);
            validate_spec(actual);
            REQUIRE(actual.elements.num_elements == input_tags.size());
            REQUIRE(actual.entities.points.size() == 2);
            REQUIRE(actual.physical_groups.size() == 2);
            REQUIRE(
                actual.entities.points[1].physical_group_tags == std::vector<std::int32_t>{7, 8});
            REQUIRE(actual.physical_groups[0].name == "first");
            REQUIRE(actual.physical_groups[1].name == "second");
        }
    }
}

TEST_CASE("MSH 2.2 derives physical tags from dimension-specific entities", "[io][v22]")
{
    using namespace mshio;
    MshSpec spec;
    spec.mesh_format.version = "2.2";
    spec.nodes = {1, 4, 1, 4, {{3, 9, 0, 4, {1, 2, 3, 4}, {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}}}};
    spec.elements = {
        4,
        4,
        1,
        4,
        {{0, 9, 15, 1, {1, 1}},
         {1, 9, 1, 1, {2, 1, 2}},
         {2, 9, 2, 1, {3, 1, 2, 3}},
         {3, 9, 4, 1, {4, 1, 2, 3, 4}}}};
    spec.entities.points.resize(1);
    spec.entities.points[0].tag = 9;
    spec.entities.points[0].physical_group_tags = {7};
    spec.entities.curves.resize(1);
    spec.entities.curves[0].tag = 9;
    spec.entities.curves[0].physical_group_tags = {8};
    spec.entities.surfaces.resize(1);
    spec.entities.surfaces[0].tag = 9;
    spec.entities.surfaces[0].physical_group_tags = {10};
    // No volume entity: absence of a physical group must be encoded as zero.
    const std::vector<std::vector<std::int32_t>> expected = {{7, 9}, {8, 9}, {10, 9}, {0, 9}};
    for (std::int32_t encoding : {0, 1}) {
        CAPTURE(encoding);
        spec.mesh_format.file_type = encoding;
        std::ostringstream out;
        save_msh(out, spec);
        REQUIRE(legacy_wire_tags(out.str(), encoding != 0) == expected);
        auto ambiguous = spec;
        ambiguous.entities.points[0].physical_group_tags = {7, 11};
        std::ostringstream rejected;
        REQUIRE_THROWS_AS(save_msh(rejected, ambiguous), mshio::UnsupportedFeature);
        ambiguous.elements.entity_blocks[0].legacy_tags = {11, 9, 1, -2};
        ambiguous.elements.entity_blocks[0].entity_tag = 12;
        std::ostringstream selected;
        save_msh(selected, ambiguous);
        auto selected_expected = expected;
        selected_expected[0] = {11, 12, 1, -2};
        REQUIRE(legacy_wire_tags(selected.str(), encoding != 0) == selected_expected);
    }
}

TEST_CASE("MSH quoted names preserve literal backslashes", "[io][strings]")
{
    using namespace mshio;
    const std::vector<std::string> names =
        {"C:\\tmp\\view", "C:\\tmp\\", "\\\\server\\share", "\\n\\t", "", " spaced name "};
    std::ostringstream source;
    source << "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n$PhysicalNames\n" << names.size() << '\n';
    std::int32_t tag = 0;
    for (const auto& name : names) source << "0 " << ++tag << " \"" << name << "\"\n";
    source << "$EndPhysicalNames\n";
    for (const auto* section : {"NodeData", "ElementData", "ElementNodeData"}) {
        source << '$' << section << '\n' << names.size() << '\n';
        for (const auto& name : names) source << '"' << name << "\"\n";
        source << "0\n3\n0\n1\n1\n1 ";
        if (std::string(section) == "ElementNodeData") source << "1 ";
        source << "2.5\n$End" << section << '\n';
    }
    std::istringstream in(source.str());
    auto spec = load_msh(in);
    REQUIRE(spec.physical_groups.size() == names.size());
    for (size_t i = 0; i < names.size(); ++i) REQUIRE(spec.physical_groups[i].name == names[i]);
    for (const auto* dataset :
         {&spec.node_data[0], &spec.element_data[0], &spec.element_node_data[0]})
        REQUIRE(dataset->header.string_tags == names);
    for (const auto* version : {"2.2", "4.1"}) {
        for (std::int32_t encoding : {0, 1}) {
            CAPTURE(version, encoding);
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = encoding;
            std::stringstream saved;
            save_msh(saved, spec);
            const auto loaded = load_msh(saved);
            for (size_t i = 0; i < names.size(); ++i)
                REQUIRE(loaded.physical_groups[i].name == names[i]);
            for (const auto* dataset :
                 {&loaded.node_data[0], &loaded.element_data[0], &loaded.element_node_data[0]}) {
                REQUIRE(dataset->header.string_tags == names);
                REQUIRE(dataset->entries[0].data == std::vector<double>{2.5});
            }
        }
    }
}

TEST_CASE("MSH rejects unterminated and unrepresentable quoted names", "[io][strings]")
{
    using namespace mshio;
    for (const auto& invalid :
         {std::string("\"unfinished"),
          std::string("\"line\nbreak\""),
          std::string("\"C:\\tmp\\")}) {
        CAPTURE(invalid);
        std::istringstream physical(
            "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n"
            "$PhysicalNames\n1\n0 1 " +
            invalid);
        REQUIRE_THROWS_AS(load_msh(physical), InvalidFormat);
        std::istringstream dataset(
            "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n"
            "$NodeData\n1\n" +
            invalid);
        REQUIRE_THROWS_AS(load_msh(dataset), InvalidFormat);
    }
    for (const auto& name :
         {std::string("embedded\"quote"),
          std::string("line\nbreak"),
          std::string("carriage\rreturn")}) {
        MshSpec spec;
        spec.physical_groups = {{0, 1, name}};
        std::ostringstream physical;
        REQUIRE_THROWS_AS(save_msh(physical, spec), InvalidFormat);
        spec.physical_groups.clear();
        Data data;
        data.header = {{name}, {}, {0, 1, 0}};
        spec.node_data = {data};
        std::ostringstream dataset;
        REQUIRE_THROWS_AS(save_msh(dataset, spec), InvalidFormat);
    }
}

TEST_CASE("Empty MSH datasets retain omitted integer tags", "[io][data-headers]")
{
    using namespace mshio;
    const std::vector<std::vector<std::int32_t>> headers = {{}, {2}, {2, 3}, {2, 3, 0}};
    for (const auto* version : {"2.2", "4.1"}) {
        for (std::int32_t input_encoding : {0, 1}) {
            CAPTURE(version, input_encoding);
            std::ostringstream source;
            source << "$MeshFormat\n" << version << ' ' << input_encoding << " 8\n";
            if (input_encoding) {
                const std::int32_t one = 1;
                source.write(reinterpret_cast<const char*>(&one), sizeof(one));
                source << '\n';
            }
            source << "$EndMeshFormat\n";
            for (const auto* section : {"NodeData", "ElementData", "ElementNodeData"}) {
                for (const auto& tags : headers) {
                    source << '$' << section << "\n0\n0\n" << tags.size() << '\n';
                    for (auto tag : tags) source << tag << '\n';
                    source << "$End" << section << '\n';
                }
            }
            // A populated dataset after the empty ones must remain readable.
            source << "$NodeData\n1\n\"populated\"\n0\n3\n0\n1\n1\n";
            if (input_encoding) {
                const std::int32_t tag = 1;
                const double value = 2.5;
                source.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
                source.write(reinterpret_cast<const char*>(&value), sizeof(value));
                source << '\n';
            } else
                source << "1 2.5\n";
            source << "$EndNodeData\n";
            std::istringstream in(source.str());
            auto spec = load_msh(in);
            REQUIRE(spec.node_data.size() == headers.size() + 1);
            REQUIRE(spec.element_data.size() == headers.size());
            REQUIRE(spec.element_node_data.size() == headers.size());
            for (std::int32_t output_encoding : {0, 1}) {
                CAPTURE(output_encoding);
                spec.mesh_format.file_type = output_encoding;
                std::stringstream saved;
                save_msh(saved, spec);
                const auto loaded = load_msh(saved);
                for (const auto* collection :
                     {&loaded.node_data, &loaded.element_data, &loaded.element_node_data}) {
                    for (size_t i = 0; i < headers.size(); ++i) {
                        REQUIRE((*collection)[i].header.string_tags.empty());
                        REQUIRE((*collection)[i].header.real_tags.empty());
                        REQUIRE((*collection)[i].header.int_tags == headers[i]);
                        REQUIRE((*collection)[i].entries.empty());
                    }
                }
                REQUIRE(
                    loaded.node_data.back().header.string_tags ==
                    std::vector<std::string>{"populated"});
                REQUIRE(loaded.node_data.back().entries[0].tag == 1);
                REQUIRE(loaded.node_data.back().entries[0].data == std::vector<double>{2.5});
            }
        }
    }
}

TEST_CASE("Nonempty MSH datasets require matching declared entry counts", "[io][data-headers]")
{
    using namespace mshio;
    for (const auto& tags : std::vector<std::vector<std::int32_t>>{{}, {0}, {0, 1}, {0, 1, 0}}) {
        for (auto member :
             {&MshSpec::node_data, &MshSpec::element_data, &MshSpec::element_node_data}) {
            MshSpec spec;
            Data data;
            data.header.int_tags = tags;
            data.entries = {{1, 1, {2.5}}};
            (spec.*member).push_back(data);
            std::ostringstream out;
            REQUIRE_THROWS_AS(save_msh(out, spec), InvalidFormat);
        }
    }
}
