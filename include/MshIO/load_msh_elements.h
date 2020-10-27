#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/io_utils.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <string>


namespace MshIO {

size_t nodes_per_element(int element_type) {
    // We support the first 15 element type so far.
    constexpr size_t element_sizes[15] = { 0, 2, 3, 4, 4, 8, 6, 5, 3, 6, 9, 10, 27, 18, 14 };
    if (element_type <= 0 || element_type >= 15) {
        throw std::ios_base::failure("Unsupported element type detected!");
    }
    return element_sizes[element_type];
}

namespace v41 {

void load_elements_ascii(std::istream& in, MshSpec& spec, std::ostream& log_out,
                      std::ostream& log_err) {
    Elements& elements = spec.elements;
    in >> elements.num_entity_blocks;
    in >> elements.num_elements;
    in >> elements.min_element_tag;
    in >> elements.max_element_tag;
    assert(in.good());

    elements.entity_blocks.resize(elements.num_entity_blocks);
    for (size_t i=0; i<elements.num_entity_blocks; i++) {
        ElementBlock& block = elements.entity_blocks[i];

        in >> block.entity_dim;
        in >> block.entity_tag;
        in >> block.element_type;
        in >> block.num_elements_in_block;

        const size_t n = nodes_per_element(block.element_type);
        block.data.resize(block.num_elements_in_block * (n+1));
        for (size_t j=0; j<block.num_elements_in_block; j++) {
            for (size_t k=0; k<=n; k++) {
                in >> block.data[j*(n+1)+k];
            }
        }
        assert(in.good());
    }
}

void load_elements_binary(std::istream& in, MshSpec& spec, std::ostream& log_out,
                      std::ostream& log_err) {
}
}

namespace v22 {
void load_elements_ascii(std::istream& in, MshSpec& spec, std::ostream& log_out,
                      std::ostream& log_err) {
}

void load_elements_binary(std::istream& in, MshSpec& spec, std::ostream& log_out,
                      std::ostream& log_err) {
}

}

void load_elements(std::istream& in, MshSpec& spec, std::ostream& log_out, std::ostream& log_err) {
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
        throw std::ios_base::failure("Fatal error: unsupported version " +
                                     version);
    }
}

}
