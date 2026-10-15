#pragma once

#include <mshio/MshSpecTopology.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/vector.h>

NB_MAKE_OPAQUE(std::vector<mshio::GhostEntity>);
NB_MAKE_OPAQUE(std::vector<mshio::PartitionedPointEntity>);
NB_MAKE_OPAQUE(std::vector<mshio::PartitionedCurveEntity>);
NB_MAKE_OPAQUE(std::vector<mshio::PartitionedSurfaceEntity>);
NB_MAKE_OPAQUE(std::vector<mshio::PartitionedVolumeEntity>);
NB_MAKE_OPAQUE(std::vector<mshio::PeriodicNodePair>);
NB_MAKE_OPAQUE(std::vector<mshio::PeriodicLink>);
NB_MAKE_OPAQUE(std::vector<mshio::GhostElement>);

namespace mshio_python_topology_detail {

template <typename Entity>
void bind_partition_fields(nanobind::class_<Entity>& binding)
{
    binding.def(nanobind::init<>())
        .def_rw("tag", &Entity::tag)
        .def_rw("parent_dim", &Entity::parent_dim)
        .def_rw("parent_tag", &Entity::parent_tag)
        .def_rw("partitions", &Entity::partitions)
        .def_rw("physical_group_tags", &Entity::physical_group_tags);
}

template <typename Entity>
void bind_partition_bounds(nanobind::class_<Entity>& binding)
{
    bind_partition_fields(binding);
    binding.def_rw("min_x", &Entity::min_x)
        .def_rw("min_y", &Entity::min_y)
        .def_rw("min_z", &Entity::min_z)
        .def_rw("max_x", &Entity::max_x)
        .def_rw("max_y", &Entity::max_y)
        .def_rw("max_z", &Entity::max_z);
}

} // namespace mshio_python_topology_detail

inline void bind_msh_topology(nanobind::module_& m)
{
    namespace nb = nanobind;
    using namespace mshio_python_topology_detail;
    nb::class_<mshio::GhostEntity>(m, "GhostEntity")
        .def(nb::init<>())
        .def_rw("tag", &mshio::GhostEntity::tag)
        .def_rw("partition", &mshio::GhostEntity::partition);
    nb::bind_vector<std::vector<mshio::GhostEntity>>(m, "GhostEntities");

    auto point = nb::class_<mshio::PartitionedPointEntity>(m, "PartitionedPointEntity");
    bind_partition_fields(point);
    point.def_rw("x", &mshio::PartitionedPointEntity::x)
        .def_rw("y", &mshio::PartitionedPointEntity::y)
        .def_rw("z", &mshio::PartitionedPointEntity::z);
    nb::bind_vector<std::vector<mshio::PartitionedPointEntity>>(m, "PartitionedPointEntities");

    auto curve = nb::class_<mshio::PartitionedCurveEntity>(m, "PartitionedCurveEntity");
    bind_partition_bounds(curve);
    curve.def_rw("boundary_point_tags", &mshio::PartitionedCurveEntity::boundary_point_tags);
    nb::bind_vector<std::vector<mshio::PartitionedCurveEntity>>(m, "PartitionedCurveEntities");

    auto surface = nb::class_<mshio::PartitionedSurfaceEntity>(m, "PartitionedSurfaceEntity");
    bind_partition_bounds(surface);
    surface.def_rw("boundary_curve_tags", &mshio::PartitionedSurfaceEntity::boundary_curve_tags);
    nb::bind_vector<std::vector<mshio::PartitionedSurfaceEntity>>(m, "PartitionedSurfaceEntities");

    auto volume = nb::class_<mshio::PartitionedVolumeEntity>(m, "PartitionedVolumeEntity");
    bind_partition_bounds(volume);
    volume.def_rw("boundary_surface_tags", &mshio::PartitionedVolumeEntity::boundary_surface_tags);
    nb::bind_vector<std::vector<mshio::PartitionedVolumeEntity>>(m, "PartitionedVolumeEntities");

    nb::class_<mshio::PartitionedEntities>(m, "PartitionedEntities")
        .def(nb::init<>())
        .def_rw("num_partitions", &mshio::PartitionedEntities::num_partitions)
        .def_rw("ghost_entities", &mshio::PartitionedEntities::ghost_entities)
        .def_rw("points", &mshio::PartitionedEntities::points)
        .def_rw("curves", &mshio::PartitionedEntities::curves)
        .def_rw("surfaces", &mshio::PartitionedEntities::surfaces)
        .def_rw("volumes", &mshio::PartitionedEntities::volumes)
        .def("empty", &mshio::PartitionedEntities::empty);

    nb::class_<mshio::PeriodicNodePair>(m, "PeriodicNodePair")
        .def(nb::init<>())
        .def_rw("slave_tag", &mshio::PeriodicNodePair::slave_tag)
        .def_rw("master_tag", &mshio::PeriodicNodePair::master_tag);
    nb::bind_vector<std::vector<mshio::PeriodicNodePair>>(m, "PeriodicNodePairs");
    nb::class_<mshio::PeriodicLink>(m, "PeriodicLink")
        .def(nb::init<>())
        .def_rw("entity_dim", &mshio::PeriodicLink::entity_dim)
        .def_rw("slave_tag", &mshio::PeriodicLink::slave_tag)
        .def_rw("master_tag", &mshio::PeriodicLink::master_tag)
        .def_rw("affine_transform", &mshio::PeriodicLink::affine_transform)
        .def_rw("node_pairs", &mshio::PeriodicLink::node_pairs);
    nb::bind_vector<std::vector<mshio::PeriodicLink>>(m, "PeriodicLinks");

    nb::class_<mshio::GhostElement>(m, "GhostElement")
        .def(nb::init<>())
        .def_rw("element_tag", &mshio::GhostElement::element_tag)
        .def_rw("partition", &mshio::GhostElement::partition)
        .def_rw("ghost_partitions", &mshio::GhostElement::ghost_partitions);
    nb::bind_vector<std::vector<mshio::GhostElement>>(m, "GhostElements");
}
