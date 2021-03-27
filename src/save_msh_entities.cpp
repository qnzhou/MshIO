#include "save_msh_entities.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

namespace mshio {

namespace v41 {

void save_entities_ascii(std::ostream& out, const MshSpec& spec)
{
    const Entities& entities = spec.entities;
    out << entities.points.size() << " " << entities.curves.size() << " "
        << entities.surfaces.size() << " " << entities.volumes.size() << std::endl;

    for (size_t i = 0; i < entities.points.size(); i++) {
        const PointEntity& point = entities.points[i];
        out << point.tag << " " << point.x << " " << point.y << " " << point.z
            << " " << point.physical_group_tags.size();
        for (size_t j = 0; j < point.physical_group_tags.size(); j++) {
            out << " " << point.physical_group_tags[j];
        }
        out << std::endl;
    }

    for (size_t i = 0; i < entities.curves.size(); i++) {
        const CurveEntity& curve = entities.curves[i];
        out << curve.tag << " " << curve.min_x << " " << curve.min_y << " "
            << curve.min_z << " " << curve.max_x << " " << curve.max_y << " "
            << curve.max_z << " " << curve.physical_group_tags.size();
        for (size_t j = 0; j < curve.physical_group_tags.size(); j++) {
            out << " " << curve.physical_group_tags[j];
        }
        out << " " << curve.boundary_point_tags.size();
        for (size_t j = 0; j < curve.boundary_point_tags.size(); j++) {
            out << " " << curve.boundary_point_tags[j];
        }
        out << std::endl;
    }

    for (size_t i = 0; i < entities.surfaces.size(); i++) {
        const SurfaceEntity& surface = entities.surfaces[i];
        out << surface.tag << " " << surface.min_x << " " << surface.min_y << " "
            << surface.min_z << " " << surface.max_x << " " << surface.max_y << " "
            << surface.max_z << " " << surface.physical_group_tags.size();
        for (size_t j = 0; j < surface.physical_group_tags.size(); j++) {
            out << " " << surface.physical_group_tags[j];
        }
        out << " " << surface.boundary_curve_tags.size();
        for (size_t j = 0; j < surface.boundary_curve_tags.size(); j++) {
            out << " " << surface.boundary_curve_tags[j];
        }
        out << std::endl;
    }

    for (size_t i = 0; i < entities.volumes.size(); i++) {
        const VolumeEntity& volume = entities.volumes[i];
        out << volume.tag << " " << volume.min_x << " " << volume.min_y << " "
            << volume.min_z << " " << volume.max_x << " " << volume.max_y << " "
            << volume.max_z << " " << volume.physical_group_tags.size();
        for (size_t j = 0; j < volume.physical_group_tags.size(); j++) {
            out << " " << volume.physical_group_tags[j];
        }
        out << " " << volume.boundary_surface_tags.size();
        for (size_t j = 0; j < volume.boundary_surface_tags.size(); j++) {
            out << " " << volume.boundary_surface_tags[j];
        }
        out << std::endl;
    }
}

void save_entities_binary(std::ostream& out, const MshSpec& spec)
{
    const Entities& entities = spec.entities;
    size_t num_points = entities.points.size();
    size_t num_curves = entities.curves.size();
    size_t num_surfaces = entities.surfaces.size();
    size_t num_volumes = entities.volumes.size();
    out.write(reinterpret_cast<const char*>(&num_points), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&num_curves), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&num_surfaces), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(&num_volumes), sizeof(size_t));

    for (size_t i = 0; i < num_points; i++) {
        const PointEntity& point = entities.points[i];
        out.write(reinterpret_cast<const char*>(&point.tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&point.x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&point.y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&point.z), sizeof(double));
        size_t num_physical_groups = point.physical_group_tags.size();
        out.write(reinterpret_cast<const char*>(&num_physical_groups), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(point.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_physical_groups));
    }

    for (size_t i = 0; i < num_curves; i++) {
        const CurveEntity& curve = entities.curves[i];
        out.write(reinterpret_cast<const char*>(&curve.tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&curve.min_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&curve.min_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&curve.min_z), sizeof(double));
        out.write(reinterpret_cast<const char*>(&curve.max_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&curve.max_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&curve.max_z), sizeof(double));
        size_t num_physical_groups = curve.physical_group_tags.size();
        out.write(reinterpret_cast<const char*>(&num_physical_groups), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(curve.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_physical_groups));
        size_t num_boundary_points = curve.boundary_point_tags.size();
        out.write(reinterpret_cast<const char*>(&num_boundary_points), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(curve.boundary_point_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_boundary_points));
    }

    for (size_t i = 0; i < num_surfaces; i++) {
        const SurfaceEntity& surface = entities.surfaces[i];
        out.write(reinterpret_cast<const char*>(&surface.tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&surface.min_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&surface.min_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&surface.min_z), sizeof(double));
        out.write(reinterpret_cast<const char*>(&surface.max_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&surface.max_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&surface.max_z), sizeof(double));
        size_t num_physical_groups = surface.physical_group_tags.size();
        out.write(reinterpret_cast<const char*>(&num_physical_groups), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(surface.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_physical_groups));
        size_t num_boundary_curves = surface.boundary_curve_tags.size();
        out.write(reinterpret_cast<const char*>(&num_boundary_curves), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(surface.boundary_curve_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_boundary_curves));
    }

    for (size_t i = 0; i < num_volumes; i++) {
        const VolumeEntity& volume = entities.volumes[i];
        out.write(reinterpret_cast<const char*>(&volume.tag), sizeof(int));
        out.write(reinterpret_cast<const char*>(&volume.min_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&volume.min_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&volume.min_z), sizeof(double));
        out.write(reinterpret_cast<const char*>(&volume.max_x), sizeof(double));
        out.write(reinterpret_cast<const char*>(&volume.max_y), sizeof(double));
        out.write(reinterpret_cast<const char*>(&volume.max_z), sizeof(double));
        size_t num_physical_groups = volume.physical_group_tags.size();
        out.write(reinterpret_cast<const char*>(&num_physical_groups), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(volume.physical_group_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_physical_groups));
        size_t num_boundary_surfaces = volume.boundary_surface_tags.size();
        out.write(reinterpret_cast<const char*>(&num_boundary_surfaces), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(volume.boundary_surface_tags.data()),
            static_cast<std::streamsize>(sizeof(size_t) * num_boundary_surfaces));
    }
}

} // namespace v41

void save_entities(std::ostream& out, const MshSpec& spec)
{
    const std::string& version = spec.mesh_format.version;
    const bool is_ascii = spec.mesh_format.file_type == 0;
    if (version == "2.2") {
        // version 2.2 has no $Entities section
        return;
    }
    out << "$Entities" << std::endl;
    if (version == "4.1") {
        if (is_ascii)
            v41::save_entities_ascii(out, spec);
        else
            v41::save_entities_binary(out, spec);
    } else {
        std::stringstream msg;
        msg << "Unsupported MSH version: " << version;
        throw UnsupportedFeature(msg.str());
    }
    out << "$EndEntities" << std::endl;
}

} // namespace mshio
