#include "load_msh_data.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>
#include "io_utils.h"

#include <fstream>
#include <iomanip>
#include <string>

namespace mshio {

namespace internal {

void load_data_header(std::istream& in, DataHeader& header)
{
    std::int32_t count;
    read_ascii(in, count);
    resize_checked(header.string_tags, nonnegative(count));
    for (auto& tag : header.string_tags) {
        if (!(in >> std::quoted(tag))) throw InvalidFormat("Truncated MSH string tag");
    }
    read_ascii(in, count);
    read_values(in, header.real_tags, nonnegative(count), false);
    read_ascii(in, count);
    read_values(in, header.int_tags, nonnegative(count), false);
}

void load_data(std::istream& in,
    Data& data,
    const std::string& version,
    bool is_binary,
    bool is_element_node_data = false)
{
    if (version != "4.1" && version != "2.2")
        throw InvalidFormat("Unsupported version " + version);
    load_data_header(in, data.header);
    if (data.header.int_tags.size() < 3)
        throw InvalidFormat("Data requires at least 3 int tags.");
    const auto fields = nonnegative(data.header.int_tags[1]);
    resize_checked(data.entries, nonnegative(data.header.int_tags[2]));
    if (is_binary) eat_white_space(in, 1);
    for (auto& entry : data.entries) {
        read_field(in, entry.tag, is_binary);
        std::uint64_t nodes = 1;
        if (is_element_node_data) {
            read_field(in, entry.num_nodes_per_element, is_binary);
            nodes = nonnegative(entry.num_nodes_per_element);
        }
        read_values(in, entry.data, checked_product(fields, nodes), is_binary);
    }
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
