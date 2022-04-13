#include "load_msh_data.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>
#include "io_utils.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <string>

namespace mshio {

namespace internal {

void load_data_header(std::istream& in, DataHeader& header)
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
void load_data_entry(
    std::istream& in, DataEntry& entry, size_t fields_per_entry, bool is_element_node_data)
{
    // TODO:
    // Based on trial and error, it seems Gmsh 4.7.1 still expect 32
    // bits tag, which is inconsistent with their spec.  Maybe
    // report a bug?
    int32_t tag_32;
    in.read(reinterpret_cast<char*>(&tag_32), 4);
    entry.tag = static_cast<size_t>(tag_32);
    // in.read(reinterpret_cast<char*>(&entry.tag), sizeof(size_t));
    if (is_element_node_data) {
        in.read(reinterpret_cast<char*>(&entry.num_nodes_per_element), sizeof(int));
        entry.data.resize(fields_per_entry * static_cast<size_t>(entry.num_nodes_per_element));
    } else {
        entry.data.resize(fields_per_entry);
    }
    in.read(reinterpret_cast<char*>(entry.data.data()),
        static_cast<std::streamsize>(sizeof(double) * entry.data.size()));
}
} // namespace v41

namespace v22 {
void load_data_entry(
    std::istream& in, DataEntry& entry, size_t fields_per_entry, bool is_element_node_data)
{
    int32_t tag_32;
    in.read(reinterpret_cast<char*>(&tag_32), 4);
    entry.tag = static_cast<size_t>(tag_32);
    if (is_element_node_data) {
        int32_t num_nodes_per_element;
        in.read(reinterpret_cast<char*>(&num_nodes_per_element), 4);
        entry.num_nodes_per_element = static_cast<int>(num_nodes_per_element);
        entry.data.resize(fields_per_entry * static_cast<size_t>(entry.num_nodes_per_element));
    } else {
        entry.data.resize(fields_per_entry);
    }
    in.read(reinterpret_cast<char*>(entry.data.data()),
        static_cast<std::streamsize>(sizeof(double) * entry.data.size()));
}
} // namespace v22

void load_data(std::istream& in,
    Data& data,
    const std::string& version,
    bool is_binary,
    bool is_element_node_data = false)
{
    load_data_header(in, data.header);

    if (data.header.int_tags.size() < 3) {
        throw InvalidFormat("Data requires at least 3 int tags.");
    }

    size_t fields_per_entry = static_cast<size_t>(data.header.int_tags[1]);
    size_t num_entries = static_cast<size_t>(data.header.int_tags[2]);
    data.entries.resize(num_entries);

    if (is_binary) {
        eat_white_space(in, 1);
        if (version == "4.1") {
            for (size_t i = 0; i < num_entries; i++) {
                v41::load_data_entry(in, data.entries[i], fields_per_entry, is_element_node_data);
            }
        } else if (version == "2.2") {
            for (size_t i = 0; i < num_entries; i++) {
                v22::load_data_entry(in, data.entries[i], fields_per_entry, is_element_node_data);
            }
        } else {
            throw InvalidFormat("Unsupported version " + version);
        }
    } else {
        for (size_t i = 0; i < num_entries; i++) {
            DataEntry& entry = data.entries[i];
            in >> entry.tag;
            if (is_element_node_data) {
                in >> entry.num_nodes_per_element;
                entry.data.resize(fields_per_entry * entry.num_nodes_per_element);
                for (size_t j = 0; j < entry.num_nodes_per_element; j++) {
                    for (size_t k = 0; k < fields_per_entry; k++) {
                        in >> entry.data[j * fields_per_entry + k];
                    }
                }
            } else {
                entry.data.resize(fields_per_entry);
                for (size_t j = 0; j < fields_per_entry; j++) {
                    in >> entry.data[j];
                }
            }
        }
    }
    assert(in.good());
}

} // namespace internal


void load_node_data(std::istream& in, MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;
    spec.node_data.emplace_back();
    internal::load_data(in, spec.node_data.back(), version, is_binary, false);
}

void load_element_data(std::istream& in, MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;
    spec.element_data.emplace_back();
    internal::load_data(in, spec.element_data.back(), version, is_binary, false);
}

void load_element_node_data(std::istream& in, MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;
    spec.element_node_data.emplace_back();
    internal::load_data(in, spec.element_node_data.back(), version, is_binary, true);
}

} // namespace mshio
