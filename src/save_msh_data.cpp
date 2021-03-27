#include "save_msh_data.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
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
    out << data.header.string_tags.size() << std::endl;
    for (const std::string& tag : data.header.string_tags) {
        out << std::quoted(tag) << std::endl;
    }

    out << data.header.real_tags.size() << std::endl;
    for (const double& tag : data.header.real_tags) {
        out << tag << std::endl;
    }

    out << data.header.int_tags.size() << std::endl;
    for (const int& tag : data.header.int_tags) {
        out << tag << std::endl;
    }

    if (is_binary) {
        if (version == "4.1") {
            int32_t tag;
            for (const DataEntry& entry : data.entries) {
                // TODO:
                // Based on trial and error, it seems Gmsh 4.7.1 still expect 32
                // bits tag, which is inconsistent with their spec.  Maybe
                // report a bug?
                tag = static_cast<int32_t>(entry.tag);
                out.write(reinterpret_cast<const char*>(&tag), 4);
                // out.write(reinterpret_cast<const char*>(&entry.tag), sizeof(size_t));
                if (is_element_node_data) {
                    out.write(
                        reinterpret_cast<const char*>(&entry.num_nodes_per_element), sizeof(int));
                }
                out.write(reinterpret_cast<const char*>(entry.data.data()),
                    static_cast<std::streamsize>(sizeof(double) * entry.data.size()));
            }
        } else if (version == "2.2") {
            int32_t tag, num_nodes_per_element;
            for (const DataEntry& entry : data.entries) {
                tag = static_cast<int32_t>(entry.tag);
                out.write(reinterpret_cast<const char*>(&tag), 4);
                if (is_element_node_data) {
                    num_nodes_per_element = static_cast<int32_t>(entry.num_nodes_per_element);
                    out.write(reinterpret_cast<const char*>(&num_nodes_per_element), 4);
                }
                out.write(reinterpret_cast<const char*>(entry.data.data()),
                    static_cast<std::streamsize>(sizeof(double) * entry.data.size()));
            }
        } else {
            throw InvalidFormat("Unsupported version " + version);
        }
    } else {
        for (const DataEntry& entry : data.entries) {
            out << entry.tag << " ";
            if (is_element_node_data) {
                out << entry.num_nodes_per_element << " ";
            }
            for (size_t i = 0; i < entry.data.size(); i++) {
                out << entry.data[i];
                if (i == entry.data.size() - 1) {
                    out << std::endl;
                } else {
                    out << ' ';
                }
            }
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
