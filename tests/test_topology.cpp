#include <mshio/exception.h>
#include <mshio/mshio.h>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

const char* topology_ascii = "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n"
                             "$PartitionedEntities\n10\n1\n31 2\n1 1 1 1\n"
                             "11 0 1 2 1 2 0.1 0.2 0.3 2 71 72\n"
                             "21 1 2 1 2 0 0 0 1 1 1 1 73 2 11 -12\n"
                             "22 2 3 1 1 0 0 0 2 2 2 0 2 21 -23\n"
                             "23 3 4 1 3 0 0 0 3 3 3 1 74 2 22 -24\n"
                             "$EndPartitionedEntities\n"
                             "$Periodic\n2\n1 21 25\n"
                             "16 1 0 0 1.25 0 1 0 0 0 0 1 0 0 0 0 1\n"
                             "2\n4294967306 4294967307\n12 13\n"
                             "0 11 12\n0\n0\n$EndPeriodic\n"
                             "$GhostElements\n2\n4294967306 1 2 2 3\n4294967307 3 0\n"
                             "$EndGhostElements\n";

std::string save(const mshio::MshSpec& spec)
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    mshio::save_msh(out, spec);
    return out.str();
}

mshio::MshSpec load(const std::string& text)
{
    std::istringstream in(text, std::ios::in | std::ios::binary);
    return mshio::load_msh(in);
}

template <typename T>
void field(std::string& bytes, T value)
{
    bytes.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

std::string binary_header()
{
    std::string bytes = "$MeshFormat\n4.1 1 8\n";
    field(bytes, std::int32_t(1));
    bytes += "\n$EndMeshFormat\n";
    return bytes;
}

void truncated_payload(const char* section, const std::string& payload)
{
    const auto header = binary_header() + "$" + section + "\n";
    for (size_t length = 0; length < payload.size(); ++length) {
        INFO(section << " truncated at " << length);
        REQUIRE_THROWS_AS(load(header + payload.substr(0, length)), mshio::InvalidFormat);
    }
}

} // namespace

TEST_CASE("Topology metadata survives ASCII and binary without a complete mesh", "[topology]")
{
    auto spec = load(topology_ascii);
    REQUIRE_NOTHROW(mshio::validate_spec(spec));
    REQUIRE(spec.partitioned_entities.num_partitions == 10);
    REQUIRE(spec.partitioned_entities.ghost_entities.at(0).tag == 31);
    REQUIRE(spec.partitioned_entities.ghost_entities.at(0).partition == 2);
    const auto& point = spec.partitioned_entities.points.at(0);
    REQUIRE(point.parent_dim == 0);
    REQUIRE(point.parent_tag == 1);
    REQUIRE(point.partitions == std::vector<std::int32_t>{1, 2});
    REQUIRE(point.x == 0.1);
    REQUIRE(point.physical_group_tags == std::vector<std::int32_t>{71, 72});
    REQUIRE(
        spec.partitioned_entities.curves.at(0).boundary_point_tags ==
        std::vector<std::int32_t>{11, -12});
    REQUIRE(
        spec.partitioned_entities.surfaces.at(0).boundary_curve_tags ==
        std::vector<std::int32_t>{21, -23});
    REQUIRE(
        spec.partitioned_entities.volumes.at(0).boundary_surface_tags ==
        std::vector<std::int32_t>{22, -24});
    REQUIRE(spec.periodic_links.at(0).affine_transform.at(3) == 1.25);
    REQUIRE(spec.periodic_links.at(0).node_pairs.at(0).slave_tag == UINT64_C(4294967306));
    REQUIRE(spec.periodic_links.at(1).affine_transform.empty());
    REQUIRE(spec.periodic_links.at(1).node_pairs.empty());
    REQUIRE(spec.ghost_elements.at(0).element_tag == UINT64_C(4294967306));
    REQUIRE(spec.ghost_elements.at(0).ghost_partitions == std::vector<std::int32_t>{2, 3});
    REQUIRE(spec.ghost_elements.at(1).ghost_partitions.empty());
    const auto expected = save(spec);
    for (auto encoding : {0, 1}) {
        spec.mesh_format.file_type = encoding;
        auto actual = load(save(spec));
        REQUIRE_NOTHROW(mshio::validate_spec(actual));
        actual.mesh_format.file_type = 0;
        REQUIRE(save(actual) == expected);
    }
}

TEST_CASE("Legacy periodic uses ASCII Affine marker even with binary mesh", "[topology]")
{
    auto spec = load("$MeshFormat\n2.2 0 8\n$EndMeshFormat\n$Periodic\n2\n"
                     "1 2 4\nAffine 1 0 0 1 0 1 0 0 0 0 1 0 0 0 0 1\n2\n1 3\n2 4\n"
                     "0 3 1\n1\n3 1\n$EndPeriodic\n");
    REQUIRE(spec.periodic_links.size() == 2);
    REQUIRE(spec.periodic_links[0].affine_transform.size() == 16);
    REQUIRE(spec.periodic_links[1].affine_transform.empty());
    REQUIRE(spec.periodic_links[1].node_pairs.at(0).slave_tag == 3);
    for (const auto* version : {"2.2", "4.1"}) {
        for (auto encoding : {0, 1}) {
            spec.mesh_format.version = version;
            spec.mesh_format.file_type = encoding;
            const auto bytes = save(spec);
            if (spec.mesh_format.version == "2.2") {
                REQUIRE(bytes.find("Affine ") != std::string::npos);
                REQUIRE(bytes.find("$Periodic\n2 \n1 2 4 ") != std::string::npos);
            }
            const auto actual = load(bytes);
            REQUIRE(
                actual.periodic_links[0].affine_transform ==
                spec.periodic_links[0].affine_transform);
            REQUIRE(actual.periodic_links[0].node_pairs[1].master_tag == 4);
            REQUIRE(actual.periodic_links[1].affine_transform.empty());
            REQUIRE(actual.periodic_links[1].node_pairs[0].master_tag == 1);
        }
    }
}

TEST_CASE("Topology output refuses lossy legacy conversions and malformed shape", "[topology]")
{
    auto spec = load(topology_ascii);
    spec.mesh_format.version = "2.2";
    REQUIRE_THROWS_AS(save(spec), mshio::UnsupportedFeature);
    spec.partitioned_entities = {};
    auto links = std::move(spec.periodic_links);
    spec.periodic_links.clear();
    REQUIRE_THROWS_AS(save(spec), mshio::UnsupportedFeature);
    spec.ghost_elements.clear();
    spec.periodic_links = std::move(links);
    REQUIRE_THROWS_AS(save(spec), mshio::InvalidFormat); // Periodic node IDs exceed int32.
    spec.mesh_format.version = "4.1";
    spec.periodic_links[0].affine_transform.pop_back();
    REQUIRE_THROWS_AS(save(spec), mshio::CorruptData);
    spec.periodic_links[0].affine_transform.clear();
    spec.periodic_links[0].entity_dim = 3;
    REQUIRE_THROWS_AS(mshio::validate_spec(spec), mshio::CorruptData);

    mshio::MshSpec empty;
    empty.mesh_format.version = "2.2";
    const auto bytes = save(empty);
    REQUIRE(bytes.find("$PartitionedEntities") == std::string::npos);
    REQUIRE(bytes.find("$Periodic") == std::string::npos);
    REQUIRE(bytes.find("$GhostElements") == std::string::npos);
}

TEST_CASE(
    "Binary topology fields have released fixed widths and reject every truncation",
    "[topology]")
{
    std::string partitioned;
    field(partitioned, UINT64_C(10)); // Whitespace-valued first byte must not be consumed.
    field(partitioned, UINT64_C(1));
    field(partitioned, std::int32_t(31));
    field(partitioned, std::int32_t(2));
    for (int i = 0; i < 4; ++i) field(partitioned, UINT64_C(1));
    for (std::int32_t dimension = 0; dimension < 4; ++dimension) {
        field(partitioned, std::int32_t(11 + dimension));
        field(partitioned, dimension);
        field(partitioned, std::int32_t(1));
        field(partitioned, UINT64_C(2));
        field(partitioned, std::int32_t(1));
        field(partitioned, std::int32_t(2));
        for (int coordinate = 0; coordinate < (dimension ? 6 : 3); ++coordinate)
            field(partitioned, double(coordinate));
        field(partitioned, UINT64_C(1));
        field(partitioned, std::int32_t(71 + dimension));
        if (dimension) {
            field(partitioned, UINT64_C(2));
            field(partitioned, std::int32_t(10 + dimension));
            field(partitioned, std::int32_t(-11 - dimension));
        }
    }
    const auto partition_spec = load(
        binary_header() + "$PartitionedEntities\n" + partitioned + "\n$EndPartitionedEntities\n");
    REQUIRE(partition_spec.partitioned_entities.num_partitions == 10);
    REQUIRE(partition_spec.partitioned_entities.ghost_entities.at(0).tag == 31);
    REQUIRE(partition_spec.partitioned_entities.points.at(0).z == 2);
    REQUIRE(partition_spec.partitioned_entities.volumes.at(0).max_z == 5);
    REQUIRE(
        partition_spec.partitioned_entities.volumes.at(0).boundary_surface_tags ==
        std::vector<std::int32_t>{13, -14});
    truncated_payload("PartitionedEntities", partitioned);

    std::string periodic;
    field(periodic, UINT64_C(1));
    field(periodic, std::int32_t(1));
    field(periodic, std::int32_t(2));
    field(periodic, std::int32_t(4));
    field(periodic, UINT64_C(16));
    for (int i = 0; i < 16; ++i) field(periodic, double(i % 5 == 0));
    field(periodic, UINT64_C(1));
    field(periodic, UINT64_C(4294967306));
    field(periodic, UINT64_C(4294967307));
    const auto periodic_spec =
        load(binary_header() + "$Periodic\n" + periodic + "\n$EndPeriodic\n");
    REQUIRE(periodic_spec.periodic_links[0].node_pairs[0].master_tag == UINT64_C(4294967307));
    REQUIRE(periodic_spec.periodic_links[0].affine_transform.size() == 16);
    REQUIRE(periodic_spec.periodic_links[0].affine_transform[15] == 1);
    truncated_payload("Periodic", periodic);

    std::string ghosts;
    field(ghosts, UINT64_C(1));
    field(ghosts, UINT64_C(4294967306));
    field(ghosts, std::int32_t(2));
    field(ghosts, UINT64_C(2));
    field(ghosts, std::int32_t(1));
    field(ghosts, std::int32_t(3));
    const auto ghost_spec =
        load(binary_header() + "$GhostElements\n" + ghosts + "\n$EndGhostElements\n");
    REQUIRE(ghost_spec.ghost_elements[0].element_tag == UINT64_C(4294967306));
    REQUIRE(ghost_spec.ghost_elements[0].ghost_partitions == std::vector<std::int32_t>{1, 3});
    truncated_payload("GhostElements", ghosts);
}

TEST_CASE("Topology readers reject hostile counts and malformed legacy fields", "[topology]")
{
    const std::string header = "$MeshFormat\n4.1 0 8\n$EndMeshFormat\n";
    for (const auto* section :
         {"$PartitionedEntities\n1 18446744073709551615\n",
          "$PartitionedEntities\n1 0 1 0 0 0 1 0 0 18446744073709551615\n",
          "$Periodic\n18446744073709551615\n",
          "$Periodic\n1 1 2 4 18446744073709551615\n",
          "$Periodic\n1 1 2 4 0 18446744073709551615\n",
          "$GhostElements\n1 1 2 18446744073709551615\n",
          "$GhostElements\n-1\n"}) {
        REQUIRE_THROWS_AS(load(header + section), mshio::InvalidFormat);
    }
    const std::string legacy = "$MeshFormat\n2.2 0 8\n$EndMeshFormat\n$Periodic\n";
    for (const auto* payload :
         {"-1\n",
          "2147483648\n",
          "1 1 2 4 Affine 1\n",
          "1 1 2 4 Affinex\n",
          "1 1 2 4 -1\n",
          "1 1 2 4 1 2147483648 1\n"}) {
        REQUIRE_THROWS_AS(load(legacy + payload), mshio::InvalidFormat);
    }
}
