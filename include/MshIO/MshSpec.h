#pragma once

#include <string>
#include <vector>

namespace MshIO {

struct MeshFormat
{
    std::string version;
    int file_type=0;
    int data_size = sizeof(size_t);
};

struct NodeBlock
{
    int entity_dim=0;
    int entity_tag=0;
    int parametric=0;
    size_t num_nodes_in_block=0;
    std::vector<size_t> tags;
    std::vector<double> data;
};

struct Nodes
{
    size_t num_entity_blocks=0;
    size_t total_num_nodes=0;
    size_t min_node_tag=0;
    size_t max_node_tag=0;
    std::vector<NodeBlock> entity_blocks;
};

struct ElementBlock
{
    int entity_dim=0;
    int entity_tag=0;
    int element_type=0;
    size_t num_elements_in_block=0;
    std::vector<size_t> data;
};

struct Elements
{
    size_t num_entity_blocks=0;
    size_t num_elements=0;
    size_t min_element_tag=0;
    size_t max_element_tag=0;
    std::vector<ElementBlock> entity_blocks;
};

struct MshSpec
{
    MeshFormat mesh_format;
    Nodes nodes;
    Elements elements;
};

} // namespace MshIO
