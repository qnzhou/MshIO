#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <mshio/MshSpecExt.h>
#include <mshio/MshSpecExtraSections.h>
#include <mshio/MshSpecTopology.h>

namespace mshio {
// MSH 4.1 format counts and identifiers are uint64_t, independently of the host ABI.
// Container allocation sizes and indices remain host size_t and must fit in memory.
static_assert(sizeof(std::uint64_t) == 8, "MSH requires eight-byte unsigned integers");
static_assert(sizeof(std::int32_t) == 4, "MSH requires four-byte signed integers");
static_assert(
    sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
    "MSH requires eight-byte IEEE 754 doubles");


struct MeshFormat
{
    std::string version = "4.1";
    std::int32_t file_type = 0;
    // MSH 4.1 supports only eight-byte format size fields, including on wasm32.
    std::int32_t data_size = 8;
};

struct NodeBlock
{
    std::int32_t entity_dim = 0;
    std::int32_t entity_tag = 0;
    std::int32_t parametric = 0;
    std::uint64_t num_nodes_in_block = 0;
    std::vector<std::uint64_t> tags;
    std::vector<double> data;
};

struct Nodes
{
    std::uint64_t num_entity_blocks = 0;
    std::uint64_t num_nodes = 0;
    std::uint64_t min_node_tag = 0;
    std::uint64_t max_node_tag = 0;
    std::vector<NodeBlock> entity_blocks;
};

struct ElementBlock
{
    std::int32_t entity_dim = 0;
    std::int32_t entity_tag = 0;
    std::int32_t element_type = 0;
    std::uint64_t num_elements_in_block = 0;
    std::vector<std::uint64_t> data;
    // MSH 2.2 integer tags shared by every element in this block. Empty means
    // derive physical membership from Entities. Loaded tags are padded to two
    // entries; entity_tag is authoritative for the second entry when saving.
    std::vector<std::int32_t> legacy_tags = {};
};

struct Elements
{
    std::uint64_t num_entity_blocks = 0;
    std::uint64_t num_elements = 0;
    std::uint64_t min_element_tag = 0;
    std::uint64_t max_element_tag = 0;
    std::vector<ElementBlock> entity_blocks;
};

struct DataHeader
{
    std::vector<std::string> string_tags; // [view name, <interpolation scheeme>]
    std::vector<double> real_tags; // [time value]
    std::vector<std::int32_t> int_tags; // [time step, num fields, num entries, partition id]
};

struct DataEntry
{
    // Gmsh postprocessing tags are signed 32-bit int fields, not mesh-size identifiers.
    std::int32_t tag = 0;
    std::int32_t num_nodes_per_element = 0; // Only used by ElementNodeData.
    std::vector<double> data;
};

struct Data
{
    DataHeader header;
    std::vector<DataEntry> entries;
};

struct PointEntity
{
    std::int32_t tag = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
};

struct CurveEntity
{
    std::int32_t tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_point_tags;
};

struct SurfaceEntity
{
    std::int32_t tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_curve_tags;
};

struct VolumeEntity
{
    std::int32_t tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_surface_tags;
};

struct Entities
{
    std::vector<PointEntity> points;
    std::vector<CurveEntity> curves;
    std::vector<SurfaceEntity> surfaces;
    std::vector<VolumeEntity> volumes;

    bool empty() const
    {
        return points.size() == 0 && curves.size() == 0 && surfaces.size() == 0 &&
               volumes.size() == 0;
    }
};

struct PhysicalGroup
{
    std::int32_t dim = 0;
    std::int32_t tag = 0;
    std::string name;
};

struct MshSpec
{
    MeshFormat mesh_format;
    Nodes nodes;
    Elements elements;
    Entities entities;
    std::vector<PhysicalGroup> physical_groups;
    std::vector<Data> node_data;
    std::vector<Data> element_data;
    std::vector<Data> element_node_data;
    PartitionedEntities partitioned_entities;
    std::vector<PeriodicLink> periodic_links;
    std::vector<GhostElement> ghost_elements;
    Parametrizations parametrizations;
    std::vector<InterpolationScheme> interpolation_schemes;

    // Custom sections
    NanoSplineFormat nanospline_format;
    std::vector<Curve> curves;
    std::vector<Patch> patches;
};

} // namespace mshio
