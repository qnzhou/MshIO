#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/io_utils.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <string>

namespace MshIO {
namespace v41 {

void load_nodes_ascii(std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Nodes& nodes = spec.nodes;
    in >> nodes.num_entity_blocks;
    in >> nodes.total_num_nodes;
    in >> nodes.min_node_tag;
    in >> nodes.max_node_tag;
    assert(in.good());
    nodes.entity_blocks.resize(nodes.num_entity_blocks);
    for (size_t i = 0; i < nodes.num_entity_blocks; i++) {
        NodeBlock& block = nodes.entity_blocks[i];
        in >> block.entity_dim;
        in >> block.entity_tag;
        in >> block.parametric;
        in >> block.num_nodes_in_block;
        assert(in.good());

        block.tags.resize(block.num_nodes_in_block);
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            in >> block.tags[j];
        }
        assert(in.good());

        assert(block.parametric >= 0 && block.parametric <= 3);
        const size_t entries_per_node = static_cast<size_t>(3 + block.parametric);
        block.data.resize(block.num_nodes_in_block * entries_per_node);
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            for (size_t k = 0; k < entries_per_node; k++) {
                in >> block.data[j * entries_per_node + k];
            }
        }
        assert(in.good());
    }
}

void load_nodes_binary(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Nodes& nodes = spec.nodes;
    eat_white_space(in);
    in.read(reinterpret_cast<char*>(&nodes.num_entity_blocks), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&nodes.total_num_nodes), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&nodes.min_node_tag), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&nodes.max_node_tag), sizeof(size_t));
    assert(in.good());
    nodes.entity_blocks.resize(nodes.num_entity_blocks);
    for (size_t i = 0; i < nodes.num_entity_blocks; i++) {
        NodeBlock& block = nodes.entity_blocks[i];
        in.read(reinterpret_cast<char*>(&block.entity_dim), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.entity_tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.parametric), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.num_nodes_in_block), sizeof(size_t));
        assert(in.good());

        block.tags.resize(block.num_nodes_in_block);
        in.read(reinterpret_cast<char*>(block.tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * block.num_nodes_in_block));
        assert(in.good());

        const size_t entries_per_node = static_cast<size_t>(3 + block.parametric);
        block.data.resize(block.num_nodes_in_block * entries_per_node);
        in.read(reinterpret_cast<char*>(block.data.data()),
            static_cast<std::streamsize>(
                sizeof(double) * block.num_nodes_in_block * entries_per_node));
        assert(in.good());
    }
}

} // namespace v41

namespace v22 {

void load_nodes_ascii(std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Nodes& nodes = spec.nodes;
    nodes.num_entity_blocks = 1;
    nodes.min_node_tag = std::numeric_limits<size_t>::max();
    nodes.max_node_tag = 0;
    in >> nodes.total_num_nodes;
    assert(in.good());
    nodes.entity_blocks.resize(1);

    NodeBlock& block = nodes.entity_blocks[0];
    block.entity_dim = 3;
    block.entity_tag = 1;
    block.parametric = 0;
    block.num_nodes_in_block = nodes.total_num_nodes;

    block.tags.resize(block.num_nodes_in_block);
    block.data.resize(block.num_nodes_in_block * 3);
    for (size_t i = 0; i < block.num_nodes_in_block; i++) {
        in >> block.tags[i];
        in >> block.data[i * 3];
        in >> block.data[i * 3 + 1];
        in >> block.data[i * 3 + 2];
    }

    if (block.num_nodes_in_block > 0) {
        nodes.min_node_tag = *std::min_element(block.tags.begin(), block.tags.end());
        nodes.max_node_tag = *std::max_element(block.tags.begin(), block.tags.end());
    }
}

void load_nodes_binary(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Nodes& nodes = spec.nodes;
    nodes.num_entity_blocks = 1;
    nodes.min_node_tag = std::numeric_limits<size_t>::max();
    nodes.max_node_tag = 0;
    in >> nodes.total_num_nodes;
    assert(in.good());
    nodes.entity_blocks.resize(1);

    NodeBlock& block = nodes.entity_blocks[0];
    block.entity_dim = 3;
    block.entity_tag = 1;
    block.parametric = 0;
    block.num_nodes_in_block = nodes.total_num_nodes;

    block.tags.resize(block.num_nodes_in_block);
    block.data.resize(block.num_nodes_in_block * 3);
    eat_white_space(in);
    for (size_t i = 0; i < block.num_nodes_in_block; i++) {
        assert(in.good());
        int tag;
        in.read(reinterpret_cast<char*>(&tag), sizeof(int));
        block.tags[i] = static_cast<size_t>(tag);
        in.read(reinterpret_cast<char*>(block.data.data() + i * 3), sizeof(double) * 3);
    }

    if (block.num_nodes_in_block > 0) {
        nodes.min_node_tag = *std::min_element(block.tags.begin(), block.tags.end());
        nodes.max_node_tag = *std::max_element(block.tags.begin(), block.tags.end());
    }
}

} // namespace v22

void load_nodes(std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (version == "4.1") {
        if (is_ascii)
            v41::load_nodes_ascii(in, spec, log_out, log_err);
        else
            v41::load_nodes_binary(in, spec, log_out, log_err);
    } else if (version == "2.2") {
        if (is_ascii)
            v22::load_nodes_ascii(in, spec, log_out, log_err);
        else
            v22::load_nodes_binary(in, spec, log_out, log_err);
    } else {
        std::stringstream msg;
        msg << "Unsupported MSH version: " <<.version;
        throw UnsupportedFeature(msg.str());
    }
}

} // namespace MshIO
