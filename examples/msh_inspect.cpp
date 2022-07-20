#include <mshio/mshio.h>
#include <iostream>
#include <iomanip>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " msh_file" << std::endl;
        return 1;
    }

    mshio::MshSpec spec = mshio::load_msh(argv[1]);

    std::cout << "sizeof(int): " << sizeof(int) << std::endl;
    std::cout << "sizeof(size_t) " << sizeof(size_t) << std::endl;
    std::cout << "sizeof(double) " << sizeof(double) << std::endl;

    std::cout << "Num point entities: " << spec.entities.points.size() << std::endl;
    for (const auto& point: spec.entities.points) {
        std::cout << "  entity tag: " << point.tag << std::endl;
        std::cout << "  entity coordinates: " << point.x <<
            " " << point.y << " " << point.z << std::endl;
        size_t num_groups = point.physical_group_tags.size();
        std::cout << "  Num physical groups: " << num_groups << std::endl;
        if (num_groups > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_groups; i++) {
                std::cout << point.physical_group_tags[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Num curve entities: " << spec.entities.curves.size() << std::endl;
    for (const auto& curve: spec.entities.curves) {
        std::cout << "  entity tag: " << curve.tag << std::endl;
        std::cout << "  bounding box min: " << curve.min_x <<
            " " << curve.min_y << " " << curve.min_z << std::endl;
        std::cout << "  bounding box max: " << curve.max_x <<
            " " << curve.max_y << " " << curve.max_z << std::endl;
        size_t num_groups = curve.physical_group_tags.size();
        std::cout << "  Num physical groups: " << num_groups << std::endl;
        if (num_groups > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_groups; i++) {
                std::cout << curve.physical_group_tags[i] << " ";
            }
            std::cout << std::endl;
        }
        size_t num_bounds = curve.boundary_point_tags.size();
        std::cout << "  Num boundary points: " << num_bounds << std::endl;
        if (num_bounds > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_bounds; i++) {
                std::cout << curve.boundary_point_tags[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Num surface entities: " << spec.entities.surfaces.size() << std::endl;
    for (const auto& surface: spec.entities.surfaces) {
        std::cout << "  entity tag: " << surface.tag << std::endl;
        std::cout << "  bounding box min: " << surface.min_x <<
            " " << surface.min_y << " " << surface.min_z << std::endl;
        std::cout << "  bounding box max: " << surface.max_x <<
            " " << surface.max_y << " " << surface.max_z << std::endl;
        size_t num_groups = surface.physical_group_tags.size();
        std::cout << "  Num physical groups: " << num_groups << std::endl;
        if (num_groups > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_groups; i++) {
                std::cout << surface.physical_group_tags[i] << " ";
            }
            std::cout << std::endl;
        }
        size_t num_bounds = surface.boundary_curve_tags.size();
        std::cout << "  Num boundary curves: " << num_bounds << std::endl;
        if (num_bounds > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_bounds; i++) {
                std::cout << surface.boundary_curve_tags[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Num volume entities: " << spec.entities.volumes.size() << std::endl;
    for (const auto& volume: spec.entities.volumes) {
        std::cout << "  entity tag: " << volume.tag << std::endl;
        std::cout << "  bounding box min: " << volume.min_x <<
            " " << volume.min_y << " " << volume.min_z << std::endl;
        std::cout << "  bounding box max: " << volume.max_x <<
            " " << volume.max_y << " " << volume.max_z << std::endl;
        size_t num_groups = volume.physical_group_tags.size();
        std::cout << "  Num physical groups: " << num_groups << std::endl;
        if (num_groups > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_groups; i++) {
                std::cout << volume.physical_group_tags[i] << " ";
            }
            std::cout << std::endl;
        }
        size_t num_bounds = volume.boundary_surface_tags.size();
        std::cout << "  Num boundary surfaces: " << num_bounds << std::endl;
        if (num_bounds > 0) {
            std::cout << "    ";
            for (size_t i = 0; i < num_bounds; i++) {
                std::cout << volume.boundary_surface_tags[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Num physical groups: " << spec.physical_groups.size() << std::endl;
    for (const auto& group: spec.physical_groups) {
        std::cout << "  group dim: " << group.dim << std::endl;
        std::cout << "  group tag: " << group.tag << std::endl;
        std::cout << "  group name: " << std::quoted(group.name) << std::endl;
    }

    std::cout << "Num nodes: " << spec.nodes.num_nodes << std::endl;
    std::cout << "Num node blocks: " << spec.nodes.num_entity_blocks << std::endl;
    std::cout << "min node tag: " << spec.nodes.min_node_tag << std::endl;
    std::cout << "max node tag: " << spec.nodes.max_node_tag << std::endl;

    for (size_t i = 0; i < spec.nodes.num_entity_blocks; i++) {
        mshio::NodeBlock& block = spec.nodes.entity_blocks[i];
        std::cout << "  entity dim: " << block.entity_dim << std::endl;
        std::cout << "  entity tag: " << block.entity_tag << std::endl;
        std::cout << "  parametric: " << block.parametric << std::endl;
        std::cout << "  num nodes in block: " << block.num_nodes_in_block << std::endl;
        for (size_t j = 0; j < block.num_nodes_in_block; j++) {
            std::cout << "    " << block.tags[j] << ": ";
            for (size_t k = 0; k < 3 + block.parametric; k++) {
                std::cout << block.data[j * (3 + block.parametric) + k] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Num elements: " << spec.elements.num_elements << std::endl;
    std::cout << "Num element blocks: " << spec.elements.num_entity_blocks << std::endl;
    std::cout << "min element tag: " << spec.elements.min_element_tag << std::endl;
    std::cout << "max element tag: " << spec.elements.max_element_tag << std::endl;
    for (size_t i = 0; i < spec.elements.num_entity_blocks; i++) {
        mshio::ElementBlock& block = spec.elements.entity_blocks[i];
        std::cout << "  entity dim: " << block.entity_dim << std::endl;
        std::cout << "  entity tag: " << block.entity_tag << std::endl;
        std::cout << "  entity type: " << block.element_type << std::endl;
        std::cout << "  num elements in block: " << block.num_elements_in_block << std::endl;

        const size_t n = mshio::nodes_per_element(block.element_type);
        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            std::cout << "    " << block.data[j * (n + 1)] << ": ";
            for (size_t k = 1; k <= n; k++) {
                std::cout << block.data[j * (n + 1) + k] << " ";
            }
            std::cout << std::endl;
        }
    }

    auto print_data = [](const mshio::Data& data) {
        std::cout << "Num string tags: " << data.header.string_tags.size() << std::endl;
        for (const std::string& tag : data.header.string_tags) {
            std::cout << std::quoted(tag) << " ";
        }
        std::cout << std::endl;

        std::cout << "Num real tags: " << data.header.real_tags.size() << std::endl;
        for (const double& tag : data.header.real_tags) {
            std::cout << tag << " ";
        }
        std::cout << std::endl;

        std::cout << "Num int tags: " << data.header.int_tags.size() << std::endl;
        for (const double& tag : data.header.int_tags) {
            std::cout << tag << " ";
        }
        std::cout << std::endl;

        size_t num_fields = static_cast<size_t>(data.header.int_tags[1]);
        size_t num_entries = static_cast<size_t>(data.header.int_tags[2]);
        size_t entry_size = data.nodes_per_element == 0 ? num_fields : num_fields * data.nodes_per_element;
        for (size_t i=0; i< num_entries; i++) {
            std::cout << "  " << data.tags[i] << ":";
            for (size_t j=0; j<entry_size; j++) {
                std::cout << ' ' << data.data[i*entry_size + j];
            }
            std::cout << std::endl;
        }
    };

    if (spec.node_data.size() > 0) {
        std::cout << "Node data: " << std::endl;
        for (const mshio::Data& data : spec.node_data) {
            print_data(data);
        }
    }

    if (spec.element_data.size() > 0) {
        std::cout << "Element data: " << std::endl;
        for (const mshio::Data& data : spec.element_data) {
            print_data(data);
        }
    }

    if (spec.element_node_data.size() > 0) {
        std::cout << "ElementNode data: " << std::endl;
        for (const mshio::Data& data : spec.element_node_data) {
            print_data(data);
        }
    }

    mshio::save_msh("tmp.msh", spec);

    return 0;
}
