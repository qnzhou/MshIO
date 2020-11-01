#pragma once

#include <MshIO/MshSpec.h>
#include <MshIO/io_utils.h>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <string>

namespace MshIO {

inline void load_data_header(std::istream& in, DataHeader& header)
{
    size_t num_string_tags, num_real_tags, num_int_tags;

    in >> num_string_tags;
    header.string_tags.resize(num_string_tags);
    for (size_t i = 0; i < num_string_tags; i++) {
        in >> std::quoted(header.string_tags[i]);
    }

    in >> num_real_tags;
    header.real_tags.resize(num_real_tags);
    for (size_t i = 0; i < num_real_tags; i++) {
        in >> header.real_tags[i];
    }

    in >> num_int_tags;
    header.int_tags.resize(num_int_tags);
    for (size_t i = 0; i < num_int_tags; i++) {
        in >> header.int_tags[i];
    }
    assert(in.good());
}

namespace v41 {
void load_data_entry(std::istream& in, size_t& tag, double* raw_data, size_t fields_per_entity)
{
    in.read(reinterpret_cast<char*>(&tag), sizeof(size_t));
    in.read(reinterpret_cast<char*>(raw_data),
        static_cast<std::streamsize>(sizeof(double) * fields_per_entity));
}
} // namespace v41

namespace v22 {
void load_data_entry(std::istream& in, size_t& tag, double* raw_data, size_t fields_per_entity)
{
    int32_t tag_32;
    in.read(reinterpret_cast<char*>(&tag_32), 4);
    tag = static_cast<size_t>(tag_32);
    in.read(reinterpret_cast<char*>(raw_data),
        static_cast<std::streamsize>(sizeof(double) * fields_per_entity));
}
} // namespace v22

inline void load_data(std::istream& in, Data& data, const std::string& version, bool is_binary)
{
    load_data_header(in, data.header);

    if (data.header.int_tags.size() < 3) {
        throw InvalidFormat("Data requires at least 3 int tags.");
    }

    size_t fields_per_entity = static_cast<size_t>(data.header.int_tags[1]);
    size_t num_entities = static_cast<size_t>(data.header.int_tags[2]);
    data.tags.resize(num_entities);
    data.data.resize(num_entities * fields_per_entity);

    if (is_binary) {
        eat_white_space(in);
        if (version == "4.1") {
            for (size_t i = 0; i < num_entities; i++) {
                v41::load_data_entry(
                    in, data.tags[i], data.data.data() + i * fields_per_entity, fields_per_entity);
            }
        } else if (version == "2.2") {
            for (size_t i = 0; i < num_entities; i++) {
                v22::load_data_entry(
                    in, data.tags[i], data.data.data() + i * fields_per_entity, fields_per_entity);
            }
        } else {
            throw InvalidFormat("Unsupported version " + version);
        }
    } else {
        for (size_t i = 0; i < num_entities; i++) {
            in >> data.tags[i];
            for (size_t j = 0; j < fields_per_entity; j++) {
                in >> data.data[i * fields_per_entity + j];
            }
        }
    }
    assert(in.good());
}


inline void load_node_data(std::istream& in, MshSpec& spec)
{
    spec.node_data.emplace_back();
    load_data(in, spec.node_data.back(), spec.mesh_format.version, spec.mesh_format.file_type > 0);
}

inline void load_element_data(std::istream& in, MshSpec& spec)
{
    spec.element_data.emplace_back();
    load_data(
        in, spec.element_data.back(), spec.mesh_format.version, spec.mesh_format.file_type > 0);
}


} // namespace MshIO
