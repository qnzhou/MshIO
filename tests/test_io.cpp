#include <catch2/catch.hpp>
#include <sstream>

#include <MshIO/load_msh.h>
#include <MshIO/MshSpec.h>
#include "test_data.h"

TEST_CASE("Load", "[loader]") {
    using namespace MshIO;
    SECTION("v2.2 ascii") {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_2.2_ascii.msh");
    }

    SECTION("v2.2 binary") {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_2.2_bin.msh");
    }

    SECTION("v4.1 ascii") {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_4.1_ascii.msh");
    }

    SECTION("v4.1 binary") {
        MshSpec spec = load_msh(MSHIO_DATA_DIR "/test_4.1_bin.msh");
    }
}
