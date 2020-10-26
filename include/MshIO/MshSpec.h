#pragma once

#include <string>
#include <vector>

namespace MshIO {

struct MeshFormat {
    std::string version;
    int file_type;
    int data_size;
};

struct NodeBlock {
    int entity_dim;
    int entity_tag;
    int parametric;
    size_t num_nodes_in_block;
    std::vector<size_t> tags;
    std::vector<double> data;
};

struct Nodes {
    size_t num_entity_blocks;
    size_t total_num_nodes;
    size_t min_node_tag;
    size_t max_node_tag;
    std::vector<NodeBlock> entity_blocks;
};

struct ElementBlock {
    int entity_dim;
    int entity_tag;
    int element_type;
    size_t num_elements_in_block;
    std::vector<size_t> tags;
    std::vector<size_t> node_tags;
};

struct Elements {
    size_t num_entity_blocks;
    size_t num_elements;
    size_t min_element_tag;
    size_t max_element_tag;
    std::vector<ElementBlock> entity_blocks;
};

struct MshSpec {
    MeshFormat mesh_format;
    Nodes nodes;
    Elements elements;
};

}  // namespace MshIO
