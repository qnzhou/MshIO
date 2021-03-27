#pragma once

#include <string>
#include <vector>

#include <mshio/MshSpecExt.h>

namespace mshio {

struct MeshFormat
{
    std::string version = "4.1";
    int file_type = 0;
    int data_size = sizeof(size_t);
};

struct NodeBlock
{
    int entity_dim = 0;
    int entity_tag = 0;
    int parametric = 0;
    size_t num_nodes_in_block = 0;
    std::vector<size_t> tags;
    std::vector<double> data;
};

struct Nodes
{
    size_t num_entity_blocks = 0;
    size_t num_nodes = 0;
    size_t min_node_tag = 0;
    size_t max_node_tag = 0;
    std::vector<NodeBlock> entity_blocks;
};

struct ElementBlock
{
    int entity_dim = 0;
    int entity_tag = 0;
    int element_type = 0;
    size_t num_elements_in_block = 0;
    std::vector<size_t> data;
};

struct Elements
{
    size_t num_entity_blocks = 0;
    size_t num_elements = 0;
    size_t min_element_tag = 0;
    size_t max_element_tag = 0;
    std::vector<ElementBlock> entity_blocks;
};

struct DataHeader
{
    std::vector<std::string> string_tags; // [view name, <interpolation scheeme>]
    std::vector<double> real_tags; // [time value]
    std::vector<int> int_tags; // [time step, num fields, num entries, partition id]
};

struct DataEntry
{
    size_t tag = 0;
    int num_nodes_per_element = 0; // Only used by ElementNodeData.
    std::vector<double> data;
};

struct Data
{
    DataHeader header;
    std::vector<DataEntry> entries;
};

struct PointEntity {
    int tag = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::vector<int> physical_group_tags;
};

struct CurveEntity {
    int tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<int> physical_group_tags;
    std::vector<int> boundary_point_tags;
};

struct SurfaceEntity {
    int tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<int> physical_group_tags;
    std::vector<int> boundary_curve_tags;
};

struct VolumeEntity {
    int tag = 0;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<int> physical_group_tags;
    std::vector<int> boundary_surface_tags;
};

struct Entities {
    std::vector<PointEntity> points;
    std::vector<CurveEntity> curves;
    std::vector<SurfaceEntity> surfaces;
    std::vector<VolumeEntity> volumes;

    bool empty() const {
        return points.size() == 0 && curves.size() == 0 && surfaces.size() == 0
            && volumes.size() == 0;
    }
};

struct PhysicalGroup
{
    int dim = 0;
    int tag = 0;
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

    // Custom sections
    NanoSplineFormat nanospline_format;
    std::vector<Curve> curves;
    std::vector<Patch> patches;
};

} // namespace mshio
