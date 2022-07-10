#include <mshio/mshio.h>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/tensor.h>

#include <cassert>

namespace nb = nanobind;

template <typename T>
using nbArray = nb::tensor<nb::numpy, T, nb::shape<nb::any>>;

template <typename T>
using nbMatrix = nb::tensor<nb::numpy, T, nb::shape<nb::any, nb::any>>;

using Int = int32_t;
using Float = double;


NB_MODULE(pymshio, m)
{
    nb::class_<mshio::MeshFormat>(m, "MeshFormat")
        .def(nb::init<>())
        .def_readwrite("version", &mshio::MeshFormat::version)
        .def_readwrite("file_type", &mshio::MeshFormat::file_type)
        .def_readwrite("data_size", &mshio::MeshFormat::data_size);

    nb::class_<mshio::NodeBlock>(m, "NodeBlock")
        .def(nb::init<>())
        .def_readwrite("entity_dim", &mshio::NodeBlock::entity_dim)
        .def_readwrite("entity_tag", &mshio::NodeBlock::entity_tag)
        .def_readwrite("parametric", &mshio::NodeBlock::parametric)
        .def_readwrite("num_nodes_in_block", &mshio::NodeBlock::num_nodes_in_block)
        .def_readwrite("_tags_", &mshio::NodeBlock::tags)
        .def_property(
            "tags",
            [](mshio::NodeBlock &block) {
                size_t shape[] = {block.tags.size()};
                return nbArray<size_t>(reinterpret_cast<void *>(block.tags.data()), 1, shape);
            },
            [](mshio::NodeBlock &block, nbArray<size_t> tensor) {
                // Memory should be managed by C++, so we need to copy numpy
                // buffer to C++.
                assert(tensor.ndim() == 1);
                assert(tensor.stride(0) == 1);
                block.tags.clear();
                block.tags.reserve(tensor.shape(0));
                for (size_t i = 0; i < tensor.shape(0); i++) {
                    block.tags.push_back(tensor(i));
                }
            })
        .def_readwrite("_data_", &mshio::NodeBlock::data)
        .def_property(
            "data",
            [](mshio::NodeBlock &block) {
                size_t shape[] = {block.num_nodes_in_block, 3};
                if (block.parametric) {
                    assert(block.entity_dim > 0);
                    shape[1] += block.entity_dim;
                }
                assert(block.data.size() == shape[0] * shape[1]);
                return nbMatrix<Float>(reinterpret_cast<void *>(block.data.data()), 2, shape);
            },
            [](mshio::NodeBlock &block, nbMatrix<Float> tensor) {
                // Memory should be managed by C++, so we need to copy numpy
                // buffer to C++.
                assert(tensor.ndim() == 2);
                size_t shape[] = {tensor.shape(0), tensor.shape(1)};
                block.data.clear();
                block.data.resize(shape[0] * shape[1]);
                for (size_t i = 0; i < shape[0]; i++) {
                    for (size_t j = 0; j < shape[1]; j++) {
                        block.data[i * shape[1] + j] = tensor(i, j);
                    }
                }
                block.num_nodes_in_block = shape[0];
            });

    nb::class_<mshio::Nodes>(m, "Nodes")
        .def(nb::init<>())
        .def_readwrite("num_entity_blocks", &mshio::Nodes::num_entity_blocks)
        .def_readwrite("num_nodes", &mshio::Nodes::num_nodes)
        .def_readwrite("min_node_tag", &mshio::Nodes::min_node_tag)
        .def_readwrite("max_node_tag", &mshio::Nodes::max_node_tag)
        .def_readwrite("entity_blocks", &mshio::Nodes::entity_blocks);

    nb::class_<mshio::ElementBlock>(m, "ElementBlock")
        .def(nb::init<>())
        .def_readwrite("entity_dim", &mshio::ElementBlock::entity_dim)
        .def_readwrite("entity_tag", &mshio::ElementBlock::entity_tag)
        .def_readwrite("element_type", &mshio::ElementBlock::element_type)
        .def_readwrite("num_elements_in_block", &mshio::ElementBlock::num_elements_in_block)
        .def_readwrite("_data_", &mshio::ElementBlock::data)
        .def_property(
            "data",
            [](mshio::ElementBlock &block) {
                assert(block.data.size() % block.num_elements_in_block == 0);
                size_t shape[] = {
                    block.num_elements_in_block, block.data.size() / block.num_elements_in_block};
                return nbMatrix<size_t>(reinterpret_cast<void *>(block.data.data()), 2, shape);
            },
            [](mshio::ElementBlock &block, nbMatrix<size_t> tensor) {
                assert(tensor.ndim() == 2);
                size_t shape[] = {tensor.shape(0), tensor.shape(1)};
                block.data.clear();
                block.data.resize(shape[0] * shape[1]);
                for (size_t i = 0; i < shape[0]; i++) {
                    for (size_t j = 0; j < shape[1]; j++) {
                        block.data[i * shape[1] + j] = tensor(i, j);
                    }
                }
                block.num_elements_in_block = tensor.shape(0);
            });

    nb::class_<mshio::Elements>(m, "Elements")
        .def(nb::init<>())
        .def_readwrite("num_entity_blocks", &mshio::Elements::num_entity_blocks)
        .def_readwrite("num_elements", &mshio::Elements::num_elements)
        .def_readwrite("min_element_tag", &mshio::Elements::min_element_tag)
        .def_readwrite("max_element_tag", &mshio::Elements::max_element_tag)
        .def_readwrite("entity_blocks", &mshio::Elements::entity_blocks);

    nb::class_<mshio::DataHeader>(m, "DataHeader")
        .def(nb::init<>())
        .def_readwrite("string_tags", &mshio::DataHeader::string_tags)
        .def_readwrite("real_tags", &mshio::DataHeader::real_tags)
        .def_readwrite("int_tags", &mshio::DataHeader::int_tags);

    nb::class_<mshio::DataEntry>(m, "DataEntry")
        .def(nb::init<>())
        .def_readwrite("tag", &mshio::DataEntry::tag)
        .def_readwrite("num_nodes_per_elements", &mshio::DataEntry::num_nodes_per_element)
        .def_readwrite("data", &mshio::DataEntry::data);

    nb::class_<mshio::Data>(m, "Data")
        .def(nb::init<>())
        .def_readwrite("header", &mshio::Data::header)
        .def_readwrite("entries", &mshio::Data::entries);

    nb::class_<mshio::PointEntity>(m, "PointEntity")
        .def(nb::init<>())
        .def_readwrite("tag", &mshio::PointEntity::tag)
        .def_readwrite("x", &mshio::PointEntity::x)
        .def_readwrite("y", &mshio::PointEntity::y)
        .def_readwrite("z", &mshio::PointEntity::z)
        .def_readwrite("physical_group_tags", &mshio::PointEntity::physical_group_tags);

    nb::class_<mshio::CurveEntity>(m, "CurveEntity")
        .def(nb::init<>())
        .def_readwrite("tag", &mshio::CurveEntity::tag)
        .def_readwrite("min_x", &mshio::CurveEntity::min_x)
        .def_readwrite("min_y", &mshio::CurveEntity::min_y)
        .def_readwrite("min_z", &mshio::CurveEntity::min_z)
        .def_readwrite("max_x", &mshio::CurveEntity::max_x)
        .def_readwrite("max_y", &mshio::CurveEntity::max_y)
        .def_readwrite("max_z", &mshio::CurveEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::CurveEntity::physical_group_tags)
        .def_readwrite("boundary_point_tags", &mshio::CurveEntity::boundary_point_tags);

    nb::class_<mshio::SurfaceEntity>(m, "SurfaceEntity")
        .def(nb::init<>())
        .def_readwrite("tag", &mshio::SurfaceEntity::tag)
        .def_readwrite("min_x", &mshio::SurfaceEntity::min_x)
        .def_readwrite("min_y", &mshio::SurfaceEntity::min_y)
        .def_readwrite("min_z", &mshio::SurfaceEntity::min_z)
        .def_readwrite("max_x", &mshio::SurfaceEntity::max_x)
        .def_readwrite("max_y", &mshio::SurfaceEntity::max_y)
        .def_readwrite("max_z", &mshio::SurfaceEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::SurfaceEntity::physical_group_tags)
        .def_readwrite("boundary_curve_tags", &mshio::SurfaceEntity::boundary_curve_tags);

    nb::class_<mshio::VolumeEntity>(m, "VolumeEntity")
        .def(nb::init<>())
        .def_readwrite("tag", &mshio::VolumeEntity::tag)
        .def_readwrite("min_x", &mshio::VolumeEntity::min_x)
        .def_readwrite("min_y", &mshio::VolumeEntity::min_y)
        .def_readwrite("min_z", &mshio::VolumeEntity::min_z)
        .def_readwrite("max_x", &mshio::VolumeEntity::max_x)
        .def_readwrite("max_y", &mshio::VolumeEntity::max_y)
        .def_readwrite("max_z", &mshio::VolumeEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::VolumeEntity::physical_group_tags)
        .def_readwrite("boundary_surface_tags", &mshio::VolumeEntity::boundary_surface_tags);

    nb::class_<mshio::Entities>(m, "Entities")
        .def(nb::init<>())
        .def("empty", &mshio::Entities::empty)
        .def_readwrite("points", &mshio::Entities::points)
        .def_readwrite("curves", &mshio::Entities::curves)
        .def_readwrite("surfaces", &mshio::Entities::surfaces)
        .def_readwrite("volumes", &mshio::Entities::volumes);

    nb::class_<mshio::PhysicalGroup>(m, "PhysicalGroup")
        .def(nb::init<>())
        .def_readwrite("dim", &mshio::PhysicalGroup::dim)
        .def_readwrite("tag", &mshio::PhysicalGroup::tag)
        .def_readwrite("name", &mshio::PhysicalGroup::name);

    nb::class_<mshio::MshSpec>(m, "MshSpec")
        .def(nb::init<>())
        .def_readwrite("mesh_format", &mshio::MshSpec::mesh_format)
        .def_readwrite("nodes", &mshio::MshSpec::nodes)
        .def_readwrite("elements", &mshio::MshSpec::elements)
        .def_readwrite("entities", &mshio::MshSpec::entities)
        .def_readwrite("physical_groups", &mshio::MshSpec::physical_groups)
        .def_readwrite("node_data", &mshio::MshSpec::node_data)
        .def_readwrite("element_data", &mshio::MshSpec::element_data)
        .def_readwrite("element_node_data", &mshio::MshSpec::element_node_data);

    m.def("load_msh", nb::overload_cast<const std::string &>(&mshio::load_msh));
    m.def("save_msh",
        nb::overload_cast<const std::string &, const mshio::MshSpec &>(&mshio::save_msh));
    m.def("validate_spec", &mshio::validate_spec);
    m.def("nodes_per_element", &mshio::nodes_per_element);
    m.def("get_element_dim", &mshio::get_element_dim);
}
