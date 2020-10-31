#include <MshIO/load_msh.h>
#include <MshIO/save_msh.h>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " msh_file" << std::endl;
        return 1;
    }

    MshIO::MshSpec spec = MshIO::load_msh(argv[1]);

    std::cout << "sizeof(int): " << sizeof(int) << std::endl;
    std::cout << "sizeof(size_t) " << sizeof(size_t) << std::endl;
    std::cout << "sizeof(double) " << sizeof(double) << std::endl;
    std::cout << "Num nodes: " << spec.nodes.total_num_nodes << std::endl;
    std::cout << "Num node blocks: " << spec.nodes.num_entity_blocks << std::endl;
    std::cout << "min node tag: " << spec.nodes.min_node_tag << std::endl;
    std::cout << "max node tag: " << spec.nodes.max_node_tag << std::endl;

    for (size_t i = 0; i < spec.nodes.num_entity_blocks; i++) {
        MshIO::NodeBlock& block = spec.nodes.entity_blocks[i];
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
        MshIO::ElementBlock& block = spec.elements.entity_blocks[i];
        std::cout << "  entity dim: " << block.entity_dim << std::endl;
        std::cout << "  entity tag: " << block.entity_tag << std::endl;
        std::cout << "  entity type: " << block.element_type << std::endl;
        std::cout << "  num elements in block: " << block.num_elements_in_block << std::endl;

        const size_t n = MshIO::nodes_per_element(block.element_type);
        for (size_t j = 0; j < block.num_elements_in_block; j++) {
            std::cout << "    " << block.data[j * (n + 1)] << ": ";
            for (size_t k = 1; k <= n; k++) {
                std::cout << block.data[j * (n + 1) + k] << " ";
            }
            std::cout << std::endl;
        }
    }

    MshIO::save_msh("tmp.msh", spec);

    return 0;
}
