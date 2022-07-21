#include "save_msh_data.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <iomanip>
#include <ostream>
#include <string>

namespace mshio {

namespace internal {

void save_data_header(std::ostream& out, const DataHeader& header)
{
    out << header.string_tags.size() << std::endl;
    for (const std::string& tag : header.string_tags) {
        out << std::quoted(tag) << std::endl;
    }

    out << header.real_tags.size() << std::endl;
    for (const double& tag : header.real_tags) {
        out << tag << std::endl;
    }

    out << header.int_tags.size() << std::endl;
    for (const int& tag : header.int_tags) {
        out << tag << std::endl;
    }
}

void save_data(std::ostream& out,
    const Data& data,
    const std::string& version,
    bool is_binary,
    bool is_element_node_data)
{
    save_data_header(out, data.header);

    const size_t num_entries = data.tags.size();
    const size_t entry_size = num_entries == 0 ? 0 : data.data.size() / num_entries;
    assert(num_entries * entry_size == data.data.size());

    if (is_binary) {
        // Note that this diviates from the MSH spec for both v2.2 and v4.1.
        // See bug report in https://github.com/qnzhou/MshIO/pull/1
        int32_t tag, num_nodes_per_element = static_cast<int32_t>(data.nodes_per_element);
        for (size_t i = 0; i < num_entries; i++) {
            tag = static_cast<int32_t>(data.tags[i]);
            out.write(reinterpret_cast<const char*>(&tag), 4);
            if (is_element_node_data) {
                out.write(reinterpret_cast<const char*>(&num_nodes_per_element), 4);
            }
            out.write(reinterpret_cast<const char*>(data.data.data() + i * entry_size),
                static_cast<std::streamsize>(sizeof(double) * entry_size));
        }
    } else {
        for (size_t i = 0; i < num_entries; i++) {
            out << data.tags[i];
            if (is_element_node_data) {
                out << ' ' << data.nodes_per_element;
            }
            for (size_t j = 0; j < entry_size; j++) {
                out << ' ' << data.data[i * entry_size + j];
            }
            out << std::endl;
        }
    }
}

} // namespace internal

void save_node_data(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;

    for (const Data& data : spec.node_data) {
        out << "$NodeData" << std::endl;
        internal::save_data(out, data, version, is_binary, false);
        out << "$EndNodeData" << std::endl;
    }
}

void save_element_data(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;

    for (const Data& data : spec.element_data) {
        out << "$ElementData" << std::endl;
        internal::save_data(out, data, version, is_binary, false);
        out << "$EndElementData" << std::endl;
    }
}

void save_element_node_data(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    bool is_binary = spec.mesh_format.file_type > 0;

    for (const Data& data : spec.element_node_data) {
        out << "$ElementNodeData" << std::endl;
        internal::save_data(out, data, version, is_binary, true);
        out << "$EndElementNodeData" << std::endl;
    }
}

} // namespace mshio
