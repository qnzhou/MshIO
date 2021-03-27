#include "save_msh_elements.h"
#include "element_utils.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>


namespace mshio {

namespace v41 {

void save_elements_ascii(std::ostream& out, const MshSpec& spec)
{
    const Elements& elements = spec.elements;
    out << elements.num_entity_blocks << " " << elements.num_elements << " "
        << elements.min_element_tag << " " << elements.max_element_tag << std::endl;

    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        const ElementBlock& block = elements.entity_blocks[i];

        out << block.entity_dim << " " << block.entity_tag << " "
            << block.element_type << " " << block.num_elements_in_block << std::endl;

        const size_t n = nodes_per_element(block.element_type);
        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            for (size_t k = 0; k <= n; k++) {
                out << block.data[j * (n + 1) + k];
                if (k == n) {
                    out << std::endl;
                } else {
                    out << ' ';
                }
            }
        }
    }
}

void save_elements_binary(std::ostream& out, const MshSpec& spec)
{
    const Elements& elements = spec.elements;
    out.write(reinterpret_cast<const char*>(&elements.num_entity_blocks), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&elements.num_elements), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&elements.min_element_tag), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&elements.max_element_tag), sizeof(size_t));

    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        const ElementBlock& block = elements.entity_blocks[i];

        out.write(reinterpret_cast<const char*>(&block.entity_dim), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.entity_tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.element_type), sizeof(int));
        out.write(reinterpret_cast<const char*>(&block.num_elements_in_block), sizeof(size_t));

        out.write(reinterpret_cast<const char*>(block.data.data()),
            static_cast<std::streamsize>(sizeof(size_t) * block.data.size()));
    }
}

} // namespace v41

namespace v22 {

void save_elements_ascii(std::ostream& out, const MshSpec& spec)
{
    const Elements& elements = spec.elements;
    out << elements.num_elements << std::endl;

    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        const ElementBlock& block = elements.entity_blocks[i];
        int element_type = block.element_type;
        const size_t n = nodes_per_element(element_type);
        constexpr int num_tags = 1;
        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            size_t element_number = block.data[j * (n + 1)];
            out << element_number << " " << element_type << " " << num_tags << " "
                << block.entity_tag << " ";
            for (size_t k = 0; k < n; k++) {
                out << block.data[j * (n + 1) + k + 1];
                if (k == n - 1) {
                    out << std::endl;
                } else {
                    out << ' ';
                }
            }
        }
    }
}

void save_elements_binary(std::ostream& out, const MshSpec& spec)
{
    const Elements& elements = spec.elements;
    out << elements.num_elements << std::endl;
    for (size_t i = 0; i < elements.num_entity_blocks; i++) {
        const ElementBlock& block = elements.entity_blocks[i];
        const int32_t element_type = block.element_type;
        constexpr int32_t num_tags = 1;
        const int32_t num_element_in_block = static_cast<int32_t>(block.num_elements_in_block);
        out.write(reinterpret_cast<const char*>(&element_type), 4);
        out.write(reinterpret_cast<const char*>(&num_element_in_block), 4);
        out.write(reinterpret_cast<const char*>(&num_tags), 4);

        const size_t n = nodes_per_element(element_type);
        const int32_t tag = static_cast<int32_t>(block.entity_tag);

        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            const int32_t element_number = static_cast<int32_t>(block.data[j * (n + 1)]);
            out.write(reinterpret_cast<const char*>(&element_number), 4);
            out.write(reinterpret_cast<const char*>(&tag), 4);
            for (size_t k = 0; k < n; k++) {
                const int32_t node_id = static_cast<int32_t>(block.data[j * (n + 1) + k + 1]);
                out.write(reinterpret_cast<const char*>(&node_id), 4);
            }
        }
    }
}

} // namespace v22

void save_elements(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;
    out << "$Elements" << std::endl;
    if (version == "4.1") {
        if (is_ascii)
            v41::save_elements_ascii(out, spec);
        else
            v41::save_elements_binary(out, spec);
    } else if (version == "2.2") {
        if (is_ascii)
            v22::save_elements_ascii(out, spec);
        else
            v22::save_elements_binary(out, spec);
    } else {
        std::stringstream msg;
        msg << "Unsupported MSH version: " << version;
        throw UnsupportedFeature(msg.str());
    }
    out << "$EndElements" << std::endl;
}


} // namespace mshio
