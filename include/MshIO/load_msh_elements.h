#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/exception.h>
#include <MshIO/io_utils.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>


namespace MshIO {

size_t nodes_per_element(int element_type)
{
    // We support the first 15 element type so far.
    constexpr size_t element_sizes[15] = {0, 2, 3, 4, 4, 8, 6, 5, 3, 6, 9, 10, 27, 18, 14};
    if (element_type <= 0 || element_type >= 15) {
        std::stringstream msg;
        msg << "Unsupported element type: " << element_type;
        throw UnsupportedFeature(msg.str());
    }
    return element_sizes[element_type];
}

namespace v41 {

void load_elements_ascii(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Elements& elements = spec.elements;
    in >> elements.num_entity_blocks;
    in >> elements.num_elements;
    in >> elements.min_element_tag;
    in >> elements.max_element_tag;
    assert(in.good());

    elements.entity_blocks.resize(elements.num_entity_blocks);
    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        ElementBlock& block = elements.entity_blocks[i];

        in >> block.entity_dim;
        in >> block.entity_tag;
        in >> block.element_type;
        in >> block.num_elements_in_block;

        const size_t n = nodes_per_element(block.element_type);
        block.data.resize(block.num_elements_in_block * (n + 1));
        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            for (size_t k = 0; k <= n; k++) {
                in >> block.data[j * (n + 1) + k];
            }
        }
        assert(in.good());
    }
}

void load_elements_binary(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    eat_white_space(in);
    Elements& elements = spec.elements;
    in.read(reinterpret_cast<char*>(&elements.num_entity_blocks), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&elements.num_elements), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&elements.min_element_tag), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&elements.max_element_tag), sizeof(size_t));
    assert(in.good());

    elements.entity_blocks.resize(elements.num_entity_blocks);
    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        ElementBlock& block = elements.entity_blocks[i];

        in.read(reinterpret_cast<char*>(&block.entity_dim), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.entity_tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.element_type), sizeof(int));
        in.read(reinterpret_cast<char*>(&block.num_elements_in_block), sizeof(size_t));

        const size_t n = nodes_per_element(block.element_type);
        block.data.resize(block.num_elements_in_block * (n + 1));
        in.read(reinterpret_cast<char*>(block.data.data()),
            static_cast<std::streamsize>(sizeof(size_t) * block.data.size()));
        assert(in.good());
    }
}
} // namespace v41

namespace v22 {
void load_elements_ascii(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Elements& elements = spec.elements;
    in >> elements.num_elements;
    elements.num_entity_blocks = elements.num_elements;
    elements.entity_blocks.resize(elements.num_elements);
    elements.min_element_tag = std::numeric_limits<size_t>::max();
    elements.max_element_tag = 0;

    int element_num, element_type;
    int num_tags;
    std::vector<int> tags;
    std::vector<int> node_ids;

    for (size_t i = 0; i < elements.num_elements; i++) {
        ElementBlock& block = elements.entity_blocks[i];
        in >> element_num;
        in >> element_type;
        in >> num_tags;
        tags.resize(static_cast<size_t>(num_tags));
        for (int j = 0; j < num_tags; j++) {
            in >> tags[static_cast<size_t>(j)];
        }

        const size_t n = nodes_per_element(element_type);
        node_ids.resize(n);
        for (size_t j = 0; j < n; j++) {
            in >> node_ids[j];
        }

        elements.min_element_tag =
            std::min(elements.min_element_tag, static_cast<size_t>(element_num));
        elements.max_element_tag =
            std::max(elements.max_element_tag, static_cast<size_t>(element_num));
        block.entity_dim = 3; // TODO: Not sure what it is for.
        block.entity_tag = 1;
        block.element_type = element_type;
        block.num_elements_in_block = 1;
        block.data.resize(n + 1);
        block.data[0] = static_cast<size_t>(element_num);

        for (size_t j = 0; j < n; j++) {
            block.data[j + 1] = static_cast<size_t>(node_ids[j]);
        }
    }
}

void load_elements_binary(
    std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    Elements& elements = spec.elements;
    in >> elements.num_elements;
    elements.entity_blocks.reserve(elements.num_elements);
    eat_white_space(in);

    std::vector<int32_t> tags, node_ids;
    int32_t min_tag = std::numeric_limits<int32_t>::max();
    int32_t max_tag = 0;
    size_t num_processed_elements = 0;
    while (num_processed_elements != elements.num_elements) {
        elements.entity_blocks.emplace_back();
        ElementBlock& block = elements.entity_blocks.back();

        int32_t element_type, num_element_in_block, num_tags, element_id;
        in.read(reinterpret_cast<char*>(&element_type), 4);
        in.read(reinterpret_cast<char*>(&num_element_in_block), 4);
        in.read(reinterpret_cast<char*>(&num_tags), 4);

        block.entity_dim = 3; // TODO: check
        block.entity_tag = 1; // TODO: check
        block.element_type = static_cast<int>(element_type);
        block.num_elements_in_block = static_cast<size_t>(num_element_in_block);

        const size_t n = nodes_per_element(element_type);
        tags.resize(static_cast<size_t>(num_tags));
        node_ids.resize(n);

        block.data.resize(block.num_elements_in_block * (n + 1));
        for (size_t i = 0; i < block.num_elements_in_block; i++) {
            in.read(reinterpret_cast<char*>(&element_id), 4);
            in.read(reinterpret_cast<char*>(tags.data()), 4 * num_tags);
            in.read(reinterpret_cast<char*>(node_ids.data()), static_cast<int>(4 * n));

            min_tag = std::min(min_tag, element_id);
            max_tag = std::max(max_tag, element_id);

            block.data[i * (n + 1)] = static_cast<size_t>(element_id);
            for (size_t j = 1; j <= n; j++) {
                block.data[i * (n + 1) + j] = static_cast<size_t>(node_ids[j - 1]);
            }
        }

        num_processed_elements += block.num_elements_in_block;
        if (num_processed_elements > elements.num_elements) {
            throw InvalidFormat("Inconsistent element count detected!");
        }
    }

    elements.num_entity_blocks = elements.entity_blocks.size();
    elements.min_element_tag = static_cast<size_t>(min_tag);
    elements.max_element_tag = static_cast<size_t>(max_tag);
}

} // namespace v22

void load_elements(std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (version == "4.1") {
        if (is_ascii)
            v41::load_elements_ascii(in, spec, log_out, log_err);
        else
            v41::load_elements_binary(in, spec, log_out, log_err);
    } else if (version == "2.2") {
        if (is_ascii)
            v22::load_elements_ascii(in, spec, log_out, log_err);
        else
            v22::load_elements_binary(in, spec, log_out, log_err);
    } else {
        std::stringstream msg;
        msg << "Unsupported MSH version: " << version;
        throw UnsupportedFeature(msg.str());
    }
}

} // namespace MshIO
