#include <iostream>
#include <MshIO/load_msh.h>

int main(int argc, char** argv) {
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

    for (size_t i=0; i<spec.nodes.num_entity_blocks; i++) {
        MshIO::NodeBlock& block = spec.nodes.entity_blocks[i];
        std::cout << "  entity dim: " << block.entity_dim << std::endl;
        std::cout << "  entity tag: " << block.entity_tag << std::endl;
        std::cout << "  parametric: " << block.parametric << std::endl;
        std::cout << "  num nodes in block: " << block.num_nodes_in_block << std::endl;
        for (size_t j=0; j<block.num_nodes_in_block; j++) {
            std::cout << "    " << block.tags[j]
                << ": ";
            for (size_t k=0; k<3+block.parametric; k++) {
                std::cout << block.data[j*(3+block.parametric)+k] << " ";
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
