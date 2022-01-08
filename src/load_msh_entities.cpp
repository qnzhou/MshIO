#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <cassert>
#include <fstream>

namespace mshio {

namespace v41 {

void load_entities_ascii(std::istream& in, MshSpec& spec)
{
    size_t num_points, num_curves, num_surfaces, num_volumes;
    in >> num_points;
    in >> num_curves;
    in >> num_surfaces;
    in >> num_volumes;
    assert(in.good());

    Entities& entities = spec.entities;
    entities.points.resize(num_points);
    entities.curves.resize(num_curves);
    entities.surfaces.resize(num_surfaces);
    entities.volumes.resize(num_volumes);

    for (size_t i = 0; i < num_points; i++) {
        PointEntity& point = entities.points[i];
        in >> point.tag;
        in >> point.x;
        in >> point.y;
        in >> point.z;
        size_t num_physical_groups;
        in >> num_physical_groups;
        point.physical_group_tags.resize(num_physical_groups);
        for (size_t j = 0; j < num_physical_groups; j++) {
            in >> point.physical_group_tags[j];
        }
    }

    for (size_t i = 0; i < num_curves; i++) {
        CurveEntity& curve = entities.curves[i];
        in >> curve.tag;
        in >> curve.min_x;
        in >> curve.min_y;
        in >> curve.min_z;
        in >> curve.max_x;
        in >> curve.max_y;
        in >> curve.max_z;
        size_t num_physical_groups;
        in >> num_physical_groups;
        curve.physical_group_tags.resize(num_physical_groups);
        for (size_t j = 0; j < num_physical_groups; j++) {
            in >> curve.physical_group_tags[j];
        }
        size_t num_boundary_points;
        in >> num_boundary_points;
        curve.boundary_point_tags.resize(num_boundary_points);
        for (size_t j = 0; j < num_boundary_points; j++) {
            in >> curve.boundary_point_tags[j];
        }
    }

    for (size_t i = 0; i < num_surfaces; i++) {
        SurfaceEntity& surface = entities.surfaces[i];
        in >> surface.tag;
        in >> surface.min_x;
        in >> surface.min_y;
        in >> surface.min_z;
        in >> surface.max_x;
        in >> surface.max_y;
        in >> surface.max_z;
        size_t num_physical_groups;
        in >> num_physical_groups;
        surface.physical_group_tags.resize(num_physical_groups);
        for (size_t j = 0; j < num_physical_groups; j++) {
            in >> surface.physical_group_tags[j];
        }
        size_t num_boundary_curves;
        in >> num_boundary_curves;
        surface.boundary_curve_tags.resize(num_boundary_curves);
        for (size_t j = 0; j < num_boundary_curves; j++) {
            in >> surface.boundary_curve_tags[j];
        }
    }

    for (size_t i = 0; i < num_volumes; i++) {
        VolumeEntity& volume = entities.volumes[i];
        in >> volume.tag;
        in >> volume.min_x;
        in >> volume.min_y;
        in >> volume.min_z;
        in >> volume.max_x;
        in >> volume.max_y;
        in >> volume.max_z;
        size_t num_physical_groups;
        in >> num_physical_groups;
        volume.physical_group_tags.resize(num_physical_groups);
        for (size_t j = 0; j < num_physical_groups; j++) {
            in >> volume.physical_group_tags[j];
        }
        size_t num_boundary_surfaces;
        in >> num_boundary_surfaces;
        volume.boundary_surface_tags.resize(num_boundary_surfaces);
        for (size_t j = 0; j < num_boundary_surfaces; j++) {
            in >> volume.boundary_surface_tags[j];
        }
    }

    assert(in.good());
}

void load_entities_binary(std::istream& in, MshSpec& spec)
{
    eat_white_space(in, 1);
    size_t num_points, num_curves, num_surfaces, num_volumes;
    in.read(reinterpret_cast<char*>(&num_points), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&num_curves), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&num_surfaces), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&num_volumes), sizeof(size_t));
    assert(in.good());

    Entities& entities = spec.entities;
    entities.points.resize(num_points);
    entities.curves.resize(num_curves);
    entities.surfaces.resize(num_surfaces);
    entities.volumes.resize(num_volumes);

    for (size_t i = 0; i < num_points; i++) {
        PointEntity& point = entities.points[i];
        in.read(reinterpret_cast<char*>(&point.tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&point.x), sizeof(double));
        in.read(reinterpret_cast<char*>(&point.y), sizeof(double));
        in.read(reinterpret_cast<char*>(&point.z), sizeof(double));
        size_t num_physical_groups;
        in.read(reinterpret_cast<char*>(&num_physical_groups), sizeof(size_t));
        assert(in.good());

        point.physical_group_tags.resize(num_physical_groups);
        in.read(reinterpret_cast<char*>(point.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_physical_groups));
        assert(in.good());
    }

    for (size_t i = 0; i < num_curves; i++) {
        CurveEntity& curve = entities.curves[i];
        in.read(reinterpret_cast<char*>(&curve.tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&curve.min_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&curve.min_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&curve.min_z), sizeof(double));
        in.read(reinterpret_cast<char*>(&curve.max_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&curve.max_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&curve.max_z), sizeof(double));
        size_t num_physical_groups;
        in.read(reinterpret_cast<char*>(&num_physical_groups), sizeof(size_t));
        assert(in.good());

        curve.physical_group_tags.resize(num_physical_groups);
        in.read(reinterpret_cast<char*>(curve.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_physical_groups));
        assert(in.good());

        size_t num_boundary_points;
        in.read(reinterpret_cast<char*>(&num_boundary_points), sizeof(size_t));
        assert(in.good());

        curve.boundary_point_tags.resize(num_boundary_points);
        in.read(reinterpret_cast<char*>(curve.boundary_point_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_boundary_points));
        assert(in.good());
    }

    for (size_t i = 0; i < num_surfaces; i++) {
        SurfaceEntity& surface = entities.surfaces[i];
        in.read(reinterpret_cast<char*>(&surface.tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&surface.min_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&surface.min_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&surface.min_z), sizeof(double));
        in.read(reinterpret_cast<char*>(&surface.max_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&surface.max_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&surface.max_z), sizeof(double));
        size_t num_physical_groups;
        in.read(reinterpret_cast<char*>(&num_physical_groups), sizeof(size_t));
        assert(in.good());

        surface.physical_group_tags.resize(num_physical_groups);
        in.read(reinterpret_cast<char*>(surface.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_physical_groups));
        assert(in.good());

        size_t num_boundary_curves;
        in.read(reinterpret_cast<char*>(&num_boundary_curves), sizeof(size_t));
        assert(in.good());

        surface.boundary_curve_tags.resize(num_boundary_curves);
        in.read(reinterpret_cast<char*>(surface.boundary_curve_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_boundary_curves));
        assert(in.good());
    }

    for (size_t i = 0; i < num_volumes; i++) {
        VolumeEntity& volume = entities.volumes[i];
        in.read(reinterpret_cast<char*>(&volume.tag), sizeof(int));
        in.read(reinterpret_cast<char*>(&volume.min_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&volume.min_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&volume.min_z), sizeof(double));
        in.read(reinterpret_cast<char*>(&volume.max_x), sizeof(double));
        in.read(reinterpret_cast<char*>(&volume.max_y), sizeof(double));
        in.read(reinterpret_cast<char*>(&volume.max_z), sizeof(double));
        size_t num_physical_groups;
        in.read(reinterpret_cast<char*>(&num_physical_groups), sizeof(size_t));
        assert(in.good());

        volume.physical_group_tags.resize(num_physical_groups);
        in.read(reinterpret_cast<char*>(volume.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_physical_groups));
        assert(in.good());

        size_t num_boundary_surfaces;
        in.read(reinterpret_cast<char*>(&num_boundary_surfaces), sizeof(size_t));
        assert(in.good());

        volume.boundary_surface_tags.resize(num_boundary_surfaces);
        in.read(reinterpret_cast<char*>(volume.boundary_surface_tags.data()),
            static_cast<std::streamsize>(sizeof(int) * num_boundary_surfaces));
        assert(in.good());
    }
}

} // namespace v41

void load_entities(std::istream& in, MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (version == "4.1") {
        if (is_ascii)
            v41::load_entities_ascii(in, spec);
        else
            v41::load_entities_binary(in, spec);
    } else if (version == "2.2") {
        throw InvalidFormat("$Entities section not supported by MSH version 2.2");
    }
}

} // namespace mshio
