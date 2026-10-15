#include "save_msh_entities.h"
#include <mshio/MshSpec.h>
#include "io_utils.h"

namespace mshio {
namespace {
template <typename T>
void save_field(std::ostream& out, const T& value, bool binary)
{
    if (binary)
        write_binary(out, &value);
    else
        out << value << ' ';
}

void save_tags(std::ostream& out, const std::vector<std::int32_t>& tags, bool binary)
{
    checked_bytes(tags.size(), sizeof(std::int32_t));
    const std::uint64_t count = tags.size();
    save_field(out, count, binary);
    if (binary)
        write_binary(out, tags.data(), count);
    else
        for (auto tag : tags) save_field(out, tag, false);
}

template <typename Entity>
void save_bounded_entity(std::ostream& out, const Entity& entity, bool binary)
{
    save_field(out, entity.tag, binary);
    save_field(out, entity.min_x, binary);
    save_field(out, entity.min_y, binary);
    save_field(out, entity.min_z, binary);
    save_field(out, entity.max_x, binary);
    save_field(out, entity.max_y, binary);
    save_field(out, entity.max_z, binary);
    save_tags(out, entity.physical_group_tags, binary);
}
} // namespace

void save_entities(std::ostream& out, const MshSpec& spec)
{
    if (spec.mesh_format.version == "2.2") return;
    if (spec.mesh_format.version != "4.1")
        throw UnsupportedFeature("Unsupported MSH version: " + spec.mesh_format.version);
    const bool binary = spec.mesh_format.file_type == 1;
    const auto& entities = spec.entities;
    const std::uint64_t points = entities.points.size(), curves = entities.curves.size(),
                        surfaces = entities.surfaces.size(), volumes = entities.volumes.size();
    out << "$Entities\n";
    save_field(out, points, binary);
    save_field(out, curves, binary);
    save_field(out, surfaces, binary);
    save_field(out, volumes, binary);
    if (!binary) out << '\n';
    for (const auto& point : entities.points) {
        save_field(out, point.tag, binary);
        save_field(out, point.x, binary);
        save_field(out, point.y, binary);
        save_field(out, point.z, binary);
        save_tags(out, point.physical_group_tags, binary);
        if (!binary) out << '\n';
    }
    for (const auto& curve : entities.curves) {
        save_bounded_entity(out, curve, binary);
        save_tags(out, curve.boundary_point_tags, binary);
        if (!binary) out << '\n';
    }
    for (const auto& surface : entities.surfaces) {
        save_bounded_entity(out, surface, binary);
        save_tags(out, surface.boundary_curve_tags, binary);
        if (!binary) out << '\n';
    }
    for (const auto& volume : entities.volumes) {
        save_bounded_entity(out, volume, binary);
        save_tags(out, volume.boundary_surface_tags, binary);
        if (!binary) out << '\n';
    }
    if (binary) out << '\n';
    out << "$EndEntities\n";
    if (!out) throw InvalidFormat("Failed to write MSH entities");
}
} // namespace mshio
