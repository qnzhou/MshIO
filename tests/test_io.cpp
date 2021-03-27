#include <catch2/catch.hpp>
#include <sstream>

#include <mshio/mshio.h>

namespace {

using namespace mshio;

void ASSERT_SAME_NODES(const MshSpec& spec1, const MshSpec& spec2) {
    const auto& nodes1 = spec1.nodes;
    const auto& nodes2 = spec2.nodes;

    REQUIRE(nodes1.num_entity_blocks == nodes2.num_entity_blocks);
    REQUIRE(nodes1.num_nodes == nodes2.num_nodes);
    REQUIRE(nodes1.min_node_tag == nodes2.min_node_tag);
    REQUIRE(nodes1.max_node_tag == nodes2.max_node_tag);

    size_t N = nodes1.num_entity_blocks;
    for (size_t i=0; i<N; i++) {
        const auto& block1 = nodes1.entity_blocks[i];
        const auto& block2 = nodes2.entity_blocks[i];

        REQUIRE(block1.entity_dim == block2.entity_dim);
        REQUIRE(block1.entity_tag == block2.entity_tag);
        REQUIRE(block1.parametric == block2.parametric);
        REQUIRE(block1.tags == block2.tags);
        REQUIRE(block1.data == block2.data);
    }
}

void ASSERT_SAME_ELEMENTS(const MshSpec& spec1, const MshSpec& spec2) {
    const auto& elements1 = spec1.elements;
    const auto& elements2 = spec2.elements;

    REQUIRE(elements1.num_entity_blocks == elements2.num_entity_blocks);
    REQUIRE(elements1.num_elements == elements2.num_elements);
    REQUIRE(elements1.min_element_tag == elements2.min_element_tag);
    REQUIRE(elements1.max_element_tag == elements2.max_element_tag);

    size_t M = elements1.num_entity_blocks;
    for (size_t i=0; i<M; i++) {
        const auto& block1 = elements1.entity_blocks[i];
        const auto& block2 = elements2.entity_blocks[i];

        REQUIRE(block1.entity_dim == block2.entity_dim);
        REQUIRE(block1.entity_tag == block2.entity_tag);
        REQUIRE(block1.element_type == block2.element_type);
        REQUIRE(block1.num_elements_in_block == block2.num_elements_in_block);
        REQUIRE(block1.data == block2.data);
    }
}

void ASSERT_SAME_CURVES(const MshSpec& spec1, const MshSpec& spec2) {
#ifdef MSHIO_EXT_NANOSPLINE
    const auto& curves1 = spec1.curves;
    const auto& curves2 = spec2.curves;

    REQUIRE(curves1.size() == curves2.size());

    const size_t N = curves1.size();
    for (size_t i=0; i<N; i++) {
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

void ASSERT_SAME(const MshSpec& spec1, const MshSpec& spec2) {
    ASSERT_SAME_NODES(spec1, spec2);
    ASSERT_SAME_ELEMENTS(spec1, spec2);

    ASSERT_SAME_CURVES(spec1, spec2);
}

void save_and_load(MshSpec& spec) {
    std::stringstream contents;
    SECTION("v4.1") {
        spec.mesh_format.version = "4.1";
        SECTION("ASCII") {
            spec.mesh_format.file_type = 0;
        }
        SECTION("Binary") {
            spec.mesh_format.file_type = 1;
        }
    }
    SECTION("v2.2") {
        spec.mesh_format.version = "2.2";
        SECTION("ASCII") {
            spec.mesh_format.file_type = 0;
        }
        SECTION("Binary") {
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

        curve.data = {
            0.0, 0.0, 0.0,
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            1.0, 1.0, 0.0
        };
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

        curve.data = {
            0.0, 0.0, 0.0, 1.0,
            1.0, 0.0, 0.0, 1.0,
            0.0, 1.0, 0.0, 1.0,
            1.0, 1.0, 0.0, 1.0
        };
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
            0.0, 0.0, 0.0, 1.0,
            1.0, 0.0, 0.0, 1.0,
            2.0, 0.0, 0.0, 1.0,
            3.0, 0.0, 0.0, 1.0,

            0.0, 1.0, 0.0, 1.0,
            1.0, 1.0, 0.0, 1.0,
            2.0, 1.0, 0.0, 1.0,
            3.0, 1.0, 0.0, 1.0,

            0.0, 2.0, 0.0, 1.0,
            1.0, 2.0, 0.0, 1.0,
            2.0, 2.0, 0.0, 1.0,
            3.0, 2.0, 0.0, 1.0,

            0.0, 3.0, 0.0, 1.0,
            1.0, 3.0, 0.0, 1.0,
            2.0, 3.0, 0.0, 1.0,
            3.0, 3.0, 0.0, 1.0
        };
    }

    validate_spec(spec);
    save_and_load(spec);
}
#endif
