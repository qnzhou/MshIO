#include <catch2/catch.hpp>
#include <sstream>

#include <MshIO/load_msh.h>
#include <MshIO/MshSpec.h>
#include "test_data.h"

TEST_CASE("MshLoader", "[loader]") {
    SECTION("Data 1") {
        std::string data = MshIO::data_1();
        std::stringstream sout(data);

        MshIO::MshSpec spec = MshIO::load_msh(sout);
    }
}
