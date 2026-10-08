#include "save_msh_data.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <iomanip>
#include <ostream>
#include <string>

namespace mshio {

namespace internal {

void save_data(std::ostream& out,
    const Data& data,
    const std::string& version,
    bool is_binary,
    bool is_element_node_data)
{
    if (version != "4.1" && version != "2.2")
        throw InvalidFormat("Unsupported version " + version);
    checked_int32(data.header.string_tags.size());
    checked_int32(data.header.real_tags.size());
    checked_int32(data.header.int_tags.size());
    if (data.header.int_tags.size() < 3)
        throw InvalidFormat("Data requires at least 3 int tags.");
    const auto fields = nonnegative(data.header.int_tags[1]);
    if (nonnegative(data.header.int_tags[2]) != data.entries.size())
        throw InvalidFormat("Inconsistent MSH data entry count");
    for (const auto& entry : data.entries) {
        const auto nodes = is_element_node_data ? nonnegative(entry.num_nodes_per_element) : 1;
        if (entry.data.size() != checked_product(fields, nodes))
            throw InvalidFormat("Inconsistent MSH data entry size");
        checked_bytes(entry.data.size(), sizeof(double));
    }
    out << data.header.string_tags.size() << '\n';
    for (const auto& tag : data.header.string_tags) out << std::quoted(tag) << '\n';
    out << data.header.real_tags.size() << '\n';
    for (auto tag : data.header.real_tags) out << tag << '\n';
    out << data.header.int_tags.size() << '\n';
    for (auto tag : data.header.int_tags) out << tag << '\n';
    for (const auto& entry : data.entries) {
        if (is_binary) {
            write_binary(out, &entry.tag);
            if (is_element_node_data) write_binary(out, &entry.num_nodes_per_element);
            write_binary(out, entry.data.data(), entry.data.size());
        } else {
            out << entry.tag << ' ';
            if (is_element_node_data) out << entry.num_nodes_per_element << ' ';
            for (auto value : entry.data) out << value << ' ';
            out << '\n';
        }
    }
    if (is_binary) out << '\n';
    if (!out) throw InvalidFormat("Failed to write MSH postprocessing data");
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
