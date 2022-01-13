#include <mshio/mshio.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<int>);
PYBIND11_MAKE_OPAQUE(std::vector<size_t>);
PYBIND11_MAKE_OPAQUE(std::vector<double>);
PYBIND11_MAKE_OPAQUE(std::vector<std::string>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::NodeBlock>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::ElementBlock>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::DataEntry>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::PointEntity>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::CurveEntity>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::SurfaceEntity>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::VolumeEntity>);
PYBIND11_MAKE_OPAQUE(std::vector<mshio::PhysicalGroup>);

PYBIND11_MODULE(mshio, m)
{
    m.doc() = "MSH format IO";
    py::bind_vector<std::vector<int>>(m, "VectorInt");
    py::bind_vector<std::vector<size_t>>(m, "VectorSizeT");
    py::bind_vector<std::vector<double>>(m, "VectorDouble");
    py::bind_vector<std::vector<std::string>>(m, "VectorString");
    py::bind_vector<std::vector<mshio::NodeBlock>>(m, "VectorNodeBlock");
    py::bind_vector<std::vector<mshio::ElementBlock>>(m, "VectorElementBlock");
    py::bind_vector<std::vector<mshio::DataEntry>>(m, "VectorDataEntry");
    py::bind_vector<std::vector<mshio::PointEntity>>(m, "VectorPointEntity");
    py::bind_vector<std::vector<mshio::CurveEntity>>(m, "VectorCurveEntity");
    py::bind_vector<std::vector<mshio::SurfaceEntity>>(m, "VectorSurfaceEntity");
    py::bind_vector<std::vector<mshio::VolumeEntity>>(m, "VectorVolumeEntity");
    py::bind_vector<std::vector<mshio::PhysicalGroup>>(m, "VectorPhysicalGroup");

    py::class_<mshio::MeshFormat>(m, "MeshFormat")
        .def(py::init<>())
        .def_readwrite("version", &mshio::MeshFormat::version)
        .def_readwrite("file_type", &mshio::MeshFormat::file_type)
        .def_readwrite("data_size", &mshio::MeshFormat::data_size);

    py::class_<mshio::NodeBlock>(m, "NodeBlock")
        .def(py::init<>())
        .def_readwrite("entity_dim", &mshio::NodeBlock::entity_dim)
        .def_readwrite("entity_tag", &mshio::NodeBlock::entity_tag)
        .def_readwrite("parametric", &mshio::NodeBlock::parametric)
        .def_readwrite("num_nodes_in_block", &mshio::NodeBlock::num_nodes_in_block)
        .def_readwrite("tags", &mshio::NodeBlock::tags)
        .def_readwrite("data", &mshio::NodeBlock::data);

    py::class_<mshio::Nodes>(m, "Nodes")
        .def(py::init<>())
        .def_readwrite("num_entity_blocks", &mshio::Nodes::num_entity_blocks)
        .def_readwrite("num_nodes", &mshio::Nodes::num_nodes)
        .def_readwrite("min_node_tag", &mshio::Nodes::min_node_tag)
        .def_readwrite("max_node_tag", &mshio::Nodes::max_node_tag)
        .def_readwrite("entity_blocks", &mshio::Nodes::entity_blocks);

    py::class_<mshio::ElementBlock>(m, "ElementBlock")
        .def(py::init<>())
        .def_readwrite("entity_dim", &mshio::ElementBlock::entity_dim)
        .def_readwrite("entity_tag", &mshio::ElementBlock::entity_tag)
        .def_readwrite("element_type", &mshio::ElementBlock::element_type)
        .def_readwrite("num_elements_in_block", &mshio::ElementBlock::num_elements_in_block)
        .def_readwrite("data", &mshio::ElementBlock::data);

    py::class_<mshio::Elements>(m, "Elements")
        .def(py::init<>())
        .def_readwrite("num_entity_blocks", &mshio::Elements::num_entity_blocks)
        .def_readwrite("num_elements", &mshio::Elements::num_elements)
        .def_readwrite("min_element_tag", &mshio::Elements::min_element_tag)
        .def_readwrite("max_element_tag", &mshio::Elements::max_element_tag)
        .def_readwrite("entity_blocks", &mshio::Elements::entity_blocks);

    py::class_<mshio::DataHeader>(m, "DataHeader")
        .def(py::init<>())
        .def_readwrite("string_tags", &mshio::DataHeader::string_tags)
        .def_readwrite("real_tags", &mshio::DataHeader::real_tags)
        .def_readwrite("int_tags", &mshio::DataHeader::int_tags);

    py::class_<mshio::DataEntry>(m, "DataEntry")
        .def(py::init<>())
        .def_readwrite("tag", &mshio::DataEntry::tag)
        .def_readwrite("num_nodes_per_elements", &mshio::DataEntry::num_nodes_per_element)
        .def_readwrite("data", &mshio::DataEntry::data);

    py::class_<mshio::Data>(m, "Data")
        .def(py::init<>())
        .def_readwrite("header", &mshio::Data::header)
        .def_readwrite("entries", &mshio::Data::entries);

    py::class_<mshio::PointEntity>(m, "PointEntity")
        .def(py::init<>())
        .def_readwrite("tag", &mshio::PointEntity::tag)
        .def_readwrite("x", &mshio::PointEntity::x)
        .def_readwrite("y", &mshio::PointEntity::y)
        .def_readwrite("z", &mshio::PointEntity::z)
        .def_readwrite("physical_group_tags", &mshio::PointEntity::physical_group_tags);

    py::class_<mshio::CurveEntity>(m, "CurveEntity")
        .def(py::init<>())
        .def_readwrite("tag", &mshio::CurveEntity::tag)
        .def_readwrite("min_x", &mshio::CurveEntity::min_x)
        .def_readwrite("min_y", &mshio::CurveEntity::min_y)
        .def_readwrite("min_z", &mshio::CurveEntity::min_z)
        .def_readwrite("max_x", &mshio::CurveEntity::max_x)
        .def_readwrite("max_y", &mshio::CurveEntity::max_y)
        .def_readwrite("max_z", &mshio::CurveEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::CurveEntity::physical_group_tags)
        .def_readwrite("boundary_point_tags", &mshio::CurveEntity::boundary_point_tags);

    py::class_<mshio::SurfaceEntity>(m, "SurfaceEntity")
        .def(py::init<>())
        .def_readwrite("tag", &mshio::SurfaceEntity::tag)
        .def_readwrite("min_x", &mshio::SurfaceEntity::min_x)
        .def_readwrite("min_y", &mshio::SurfaceEntity::min_y)
        .def_readwrite("min_z", &mshio::SurfaceEntity::min_z)
        .def_readwrite("max_x", &mshio::SurfaceEntity::max_x)
        .def_readwrite("max_y", &mshio::SurfaceEntity::max_y)
        .def_readwrite("max_z", &mshio::SurfaceEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::SurfaceEntity::physical_group_tags)
        .def_readwrite("boundary_curve_tags", &mshio::SurfaceEntity::boundary_curve_tags);

    py::class_<mshio::VolumeEntity>(m, "VolumeEntity")
        .def(py::init<>())
        .def_readwrite("tag", &mshio::VolumeEntity::tag)
        .def_readwrite("min_x", &mshio::VolumeEntity::min_x)
        .def_readwrite("min_y", &mshio::VolumeEntity::min_y)
        .def_readwrite("min_z", &mshio::VolumeEntity::min_z)
        .def_readwrite("max_x", &mshio::VolumeEntity::max_x)
        .def_readwrite("max_y", &mshio::VolumeEntity::max_y)
        .def_readwrite("max_z", &mshio::VolumeEntity::max_z)
        .def_readwrite("physical_group_tags", &mshio::VolumeEntity::physical_group_tags)
        .def_readwrite("boundary_surface_tags", &mshio::VolumeEntity::boundary_surface_tags);

    py::class_<mshio::Entities>(m, "Entities")
        .def(py::init<>())
        .def("empty", &mshio::Entities::empty)
        .def_readwrite("points", &mshio::Entities::points)
        .def_readwrite("curves", &mshio::Entities::curves)
        .def_readwrite("surfaces", &mshio::Entities::surfaces)
        .def_readwrite("volumes", &mshio::Entities::volumes);

    py::class_<mshio::PhysicalGroup>(m, "PhysicalGroup")
        .def(py::init<>())
        .def_readwrite("dim", &mshio::PhysicalGroup::dim)
        .def_readwrite("tag", &mshio::PhysicalGroup::tag)
        .def_readwrite("name", &mshio::PhysicalGroup::name);

    py::class_<mshio::MshSpec>(m, "MshSpec")
        .def(py::init<>())
        .def_readwrite("mesh_format", &mshio::MshSpec::mesh_format)
        .def_readwrite("nodes", &mshio::MshSpec::nodes)
        .def_readwrite("elements", &mshio::MshSpec::elements)
        .def_readwrite("entities", &mshio::MshSpec::entities)
        .def_readwrite("physical_groups", &mshio::MshSpec::physical_groups)
        .def_readwrite("node_data", &mshio::MshSpec::node_data)
        .def_readwrite("element_data", &mshio::MshSpec::element_data)
        .def_readwrite("element_node_data", &mshio::MshSpec::element_node_data);
}
