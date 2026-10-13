#include "portability_fixture.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void export_fixtures(const std::string& directory)
{
    for (bool sparse : {false, true}) {
        auto spec = mshio::load_msh(MSHIO_DATA_DIR "/portability_4.1_binary.msh");
        portability::same(spec, portability::expected_fixture());
        if (sparse) {
            spec.elements.min_element_tag = 13;
            spec.elements.max_element_tag = portability::sparse_element_tag;
            spec.elements.entity_blocks.front().data[5] = portability::sparse_element_tag;
        }
        const auto expected = portability::expected_fixture(sparse);
        portability::same(spec, expected);
        for (std::int32_t encoding : {0, 1}) {
            spec.mesh_format.file_type = encoding;
            const auto path = directory + "/portability" + (sparse ? "_sparse" : "") +
                (encoding == 0 ? "_ascii.msh" : "_binary.msh");
            mshio::save_msh(path, spec);
            const auto actual = mshio::load_msh(path);
            portability::same(actual, expected);
            mshio::validate_spec(actual);
            std::cout << "Exported and checked " << path << '\n';
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc != 2) throw std::runtime_error("Usage: mshio_fixture_export OUTPUT_DIRECTORY");
        export_fixtures(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
