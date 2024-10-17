#include <mshio/mshio.h>


#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

NB_MODULE(pymshio, m)
{
    nb::class_<mshio::MeshFormat>(m, "MeshFormat")
        .def(nb::init<>())
        .def_rw("version", &mshio::MeshFormat::version)
        .def_rw("file_type", &mshio::MeshFormat::file_type)
        .def_rw("data_size", &mshio::MeshFormat::data_size);

    nb::class_<mshio::NodeBlock>(m, "NodeBlock")
        .def(nb::init<>())
        .def_rw("entity_dim", &mshio::NodeBlock::entity_dim)
        .def_rw("entity_tag", &mshio::NodeBlock::entity_tag)
        .def_rw("parametric", &mshio::NodeBlock::parametric)
        .def_rw("num_nodes_in_block", &mshio::NodeBlock::num_nodes_in_block)
        .def_rw("tags", &mshio::NodeBlock::tags)
        .def_rw("data", &mshio::NodeBlock::data);

    nb::class_<mshio::Nodes>(m, "Nodes")
        .def(nb::init<>())
        .def_rw("num_entity_blocks", &mshio::Nodes::num_entity_blocks)
        .def_rw("num_nodes", &mshio::Nodes::num_nodes)
        .def_rw("min_node_tag", &mshio::Nodes::min_node_tag)
        .def_rw("max_node_tag", &mshio::Nodes::max_node_tag)
        .def_rw("entity_blocks", &mshio::Nodes::entity_blocks);

    nb::class_<mshio::ElementBlock>(m, "ElementBlock")
        .def(nb::init<>())
        .def_rw("entity_dim", &mshio::ElementBlock::entity_dim)
        .def_rw("entity_tag", &mshio::ElementBlock::entity_tag)
        .def_rw("element_type", &mshio::ElementBlock::element_type)
        .def_rw("num_elements_in_block", &mshio::ElementBlock::num_elements_in_block)
        .def_rw("data", &mshio::ElementBlock::data);

    nb::class_<mshio::Elements>(m, "Elements")
        .def(nb::init<>())
        .def_rw("num_entity_blocks", &mshio::Elements::num_entity_blocks)
        .def_rw("num_elements", &mshio::Elements::num_elements)
        .def_rw("min_element_tag", &mshio::Elements::min_element_tag)
        .def_rw("max_element_tag", &mshio::Elements::max_element_tag)
        .def_rw("entity_blocks", &mshio::Elements::entity_blocks);

    nb::class_<mshio::DataHeader>(m, "DataHeader")
        .def(nb::init<>())
        .def_rw("string_tags", &mshio::DataHeader::string_tags)
        .def_rw("real_tags", &mshio::DataHeader::real_tags)
        .def_rw("int_tags", &mshio::DataHeader::int_tags);

    nb::class_<mshio::DataEntry>(m, "DataEntry")
        .def(nb::init<>())
        .def_rw("tag", &mshio::DataEntry::tag)
        .def_rw("num_nodes_per_elements", &mshio::DataEntry::num_nodes_per_element)
        .def_rw("data", &mshio::DataEntry::data);

    nb::class_<mshio::Data>(m, "Data")
        .def(nb::init<>())
        .def_rw("header", &mshio::Data::header)
        .def_rw("entries", &mshio::Data::entries);

    nb::class_<mshio::PointEntity>(m, "PointEntity")
        .def(nb::init<>())
        .def_rw("tag", &mshio::PointEntity::tag)
        .def_rw("x", &mshio::PointEntity::x)
        .def_rw("y", &mshio::PointEntity::y)
        .def_rw("z", &mshio::PointEntity::z)
        .def_rw("physical_group_tags", &mshio::PointEntity::physical_group_tags);

    nb::class_<mshio::CurveEntity>(m, "CurveEntity")
        .def(nb::init<>())
        .def_rw("tag", &mshio::CurveEntity::tag)
        .def_rw("min_x", &mshio::CurveEntity::min_x)
        .def_rw("min_y", &mshio::CurveEntity::min_y)
        .def_rw("min_z", &mshio::CurveEntity::min_z)
        .def_rw("max_x", &mshio::CurveEntity::max_x)
        .def_rw("max_y", &mshio::CurveEntity::max_y)
        .def_rw("max_z", &mshio::CurveEntity::max_z)
        .def_rw("physical_group_tags", &mshio::CurveEntity::physical_group_tags)
        .def_rw("boundary_point_tags", &mshio::CurveEntity::boundary_point_tags);

    nb::class_<mshio::SurfaceEntity>(m, "SurfaceEntity")
        .def(nb::init<>())
        .def_rw("tag", &mshio::SurfaceEntity::tag)
        .def_rw("min_x", &mshio::SurfaceEntity::min_x)
        .def_rw("min_y", &mshio::SurfaceEntity::min_y)
        .def_rw("min_z", &mshio::SurfaceEntity::min_z)
        .def_rw("max_x", &mshio::SurfaceEntity::max_x)
        .def_rw("max_y", &mshio::SurfaceEntity::max_y)
        .def_rw("max_z", &mshio::SurfaceEntity::max_z)
        .def_rw("physical_group_tags", &mshio::SurfaceEntity::physical_group_tags)
        .def_rw("boundary_curve_tags", &mshio::SurfaceEntity::boundary_curve_tags);

    nb::class_<mshio::VolumeEntity>(m, "VolumeEntity")
        .def(nb::init<>())
        .def_rw("tag", &mshio::VolumeEntity::tag)
        .def_rw("min_x", &mshio::VolumeEntity::min_x)
        .def_rw("min_y", &mshio::VolumeEntity::min_y)
        .def_rw("min_z", &mshio::VolumeEntity::min_z)
        .def_rw("max_x", &mshio::VolumeEntity::max_x)
        .def_rw("max_y", &mshio::VolumeEntity::max_y)
        .def_rw("max_z", &mshio::VolumeEntity::max_z)
        .def_rw("physical_group_tags", &mshio::VolumeEntity::physical_group_tags)
        .def_rw("boundary_surface_tags", &mshio::VolumeEntity::boundary_surface_tags);

    nb::class_<mshio::Entities>(m, "Entities")
        .def(nb::init<>())
        .def("empty", &mshio::Entities::empty)
        .def_rw("points", &mshio::Entities::points)
        .def_rw("curves", &mshio::Entities::curves)
        .def_rw("surfaces", &mshio::Entities::surfaces)
        .def_rw("volumes", &mshio::Entities::volumes);

    nb::class_<mshio::PhysicalGroup>(m, "PhysicalGroup")
        .def(nb::init<>())
        .def_rw("dim", &mshio::PhysicalGroup::dim)
        .def_rw("tag", &mshio::PhysicalGroup::tag)
        .def_rw("name", &mshio::PhysicalGroup::name);

    nb::class_<mshio::MshSpec>(m, "MshSpec")
        .def(nb::init<>())
        .def_rw("mesh_format", &mshio::MshSpec::mesh_format)
        .def_rw("nodes", &mshio::MshSpec::nodes)
        .def_rw("elements", &mshio::MshSpec::elements)
        .def_rw("entities", &mshio::MshSpec::entities)
        .def_rw("physical_groups", &mshio::MshSpec::physical_groups)
        .def_rw("node_data", &mshio::MshSpec::node_data)
        .def_rw("element_data", &mshio::MshSpec::element_data)
        .def_rw("element_node_data", &mshio::MshSpec::element_node_data);

    m.def("load_msh", nb::overload_cast<const std::string&>(&mshio::load_msh));
    m.def(
        "save_msh", nb::overload_cast<const std::string&, const mshio::MshSpec&>(&mshio::save_msh));
    m.def("validate_spec", &mshio::validate_spec);
    m.def("nodes_per_element", &mshio::nodes_per_element);
    m.def("get_element_dim", &mshio::get_element_dim);
}
