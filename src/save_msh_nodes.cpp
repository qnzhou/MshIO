#include "save_msh_nodes.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <ostream>
#include <sstream>

namespace mshio {
namespace v41 {

void save_nodes_ascii(std::ostream& out, const MshSpec& spec)
{
    const Nodes& nodes = spec.nodes;
    out << nodes.num_entity_blocks << " " << nodes.num_nodes << " "
        << nodes.min_node_tag << " " << nodes.max_node_tag << std::endl;

    for (size_t i = 0; i < nodes.num_entity_blocks; i++) {
        const NodeBlock& block = nodes.entity_blocks[i];
        out << block.entity_dim << " " << block.entity_tag << " " << block.parametric << " "
            << block.num_nodes_in_block << std::endl;
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            out << block.tags[j] << std::endl;
        }
        const size_t entries_per_node =
            static_cast<size_t>(3 + ((block.parametric == 1) ? block.entity_dim : 0));
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            for (size_t k = 0; k < entries_per_node; k++) {
                out << block.data[j * entries_per_node + k];
                if (k == entries_per_node - 1) {
                    out << std::endl;
                } else {
                    out << ' ';
                }
            }
        }
    }
}

void save_nodes_binary(std::ostream& out, const MshSpec& spec)
{
    const Nodes& nodes = spec.nodes;
    out.write(reinterpret_cast<const char*>(&nodes.num_entity_blocks), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&nodes.num_nodes), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&nodes.min_node_tag), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&nodes.max_node_tag), sizeof(size_t));

    for (size_t i = 0; i < nodes.num_entity_blocks; i++) {
        const NodeBlock& block = nodes.entity_blocks[i];
        out.write(reinterpret_cast<const char*>(&block.entity_dim), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.entity_tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.parametric), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.num_nodes_in_block), sizeof(size_t));

        out.write(reinterpret_cast<const char*>(block.tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * block.num_nodes_in_block));

        const size_t entries_per_node =
            static_cast<size_t>(3 + ((block.parametric == 1) ? block.entity_dim : 0));
        out.write(reinterpret_cast<const char*>(block.data.data()),
            static_cast<std::streamsize>(
                sizeof(double) * block.num_nodes_in_block * entries_per_node));
    }
}

} // namespace v41

namespace v22 {

void save_nodes_ascii(std::ostream& out, const MshSpec& spec)
{
    const Nodes& nodes = spec.nodes;
    out << nodes.num_nodes << std::endl;

    for (size_t i=0; i<nodes.num_entity_blocks; i++) {
        const auto& block = nodes.entity_blocks[i];
        const size_t entries_per_node =
            static_cast<size_t>(3 + ((block.parametric == 1) ? block.entity_dim : 0));
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            out << block.tags[j] << " " << block.data[j * entries_per_node] << " "
                << block.data[j * entries_per_node + 1] << " "
                << block.data[j * entries_per_node + 2] << std::endl;
        }
    }
}

void save_nodes_binary(std::ostream& out, const MshSpec& spec)
{
    const Nodes& nodes = spec.nodes;
    out << nodes.num_nodes << std::endl;

    for (size_t i=0; i<nodes.num_entity_blocks; i++) {
        const auto& block = nodes.entity_blocks[i];
        const size_t entries_per_node =
            static_cast<size_t>(3 + ((block.parametric == 1) ? block.entity_dim : 0));
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            const int32_t node_id = static_cast<int32_t>(block.tags[j]);
            out.write(reinterpret_cast<const char*>(&node_id), 4);
            out.write(reinterpret_cast<const char*>(block.data.data() + j * entries_per_node),
                static_cast<std::streamsize>(sizeof(double) * 3));
        }
    }
}

} // namespace v22

void save_nodes(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;

    out << "$Nodes" << std::endl;
    if (version == "4.1") {
        if (is_ascii)
            v41::save_nodes_ascii(out, spec);
        else
            v41::save_nodes_binary(out, spec);
    } else if (version == "2.2") {
        if (is_ascii)
            v22::save_nodes_ascii(out, spec);
        else
            v22::save_nodes_binary(out, spec);
    } else {
        std::stringstream msg;
        msg << "Unsupported MSH version: " << version;
        throw UnsupportedFeature(msg.str());
    }
    out << "$EndNodes" << std::endl;
}

} // namespace mshio
