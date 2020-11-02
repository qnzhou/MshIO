#include <catch2/catch.hpp>
#include <sstream>

#include <MshIO/mshio.h>

TEST_CASE("Load", "[loader]")
{
    using namespace mshio;
    SECTION("v2.2 ascii")
    {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_2.2_ascii.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v2.2 binary")
    {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_2.2_bin.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v4.1 ascii")
    {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_4.1_ascii.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }

    SECTION("v4.1 binary")
    {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_4.1_bin.msh");
        validate_spec(spec);
        REQUIRE(spec.nodes.num_nodes == 6);
        REQUIRE(spec.elements.num_elements == 2);
    }
}
