#include "load_msh_entities.h"
#include <mshio/MshSpec.h>
#include "io_utils.h"

namespace mshio {
namespace {
void load_tags(std::istream& in, std::vector<std::int32_t>& tags, bool binary)
{
    std::uint64_t count;
    read_field(in, count, binary);
    read_values(in, tags, count, binary);
}

template <typename Entity>
void load_bounded_entity(std::istream& in, Entity& entity, bool binary)
{
    read_field(in, entity.tag, binary);
    read_field(in, entity.min_x, binary);
    read_field(in, entity.min_y, binary);
    read_field(in, entity.min_z, binary);
    read_field(in, entity.max_x, binary);
    read_field(in, entity.max_y, binary);
    read_field(in, entity.max_z, binary);
    load_tags(in, entity.physical_group_tags, binary);
}
} // namespace

void load_entities(std::istream& in, MshSpec& spec)
{
    if (spec.mesh_format.version != "4.1")
        throw InvalidFormat("$Entities requires MSH version 4.1");
    const bool binary = spec.mesh_format.file_type == 1;
    if (binary) eat_white_space(in, 1);
    std::uint64_t points, curves, surfaces, volumes;
    read_field(in, points, binary);
    read_field(in, curves, binary);
    read_field(in, surfaces, binary);
    read_field(in, volumes, binary);
    auto& entities = spec.entities;
    read_records(entities.points, points, [&](PointEntity& point) {
        read_field(in, point.tag, binary);
        read_field(in, point.x, binary);
        read_field(in, point.y, binary);
        read_field(in, point.z, binary);
        load_tags(in, point.physical_group_tags, binary);
    });
    read_records(entities.curves, curves, [&](CurveEntity& curve) {
        load_bounded_entity(in, curve, binary);
        load_tags(in, curve.boundary_point_tags, binary);
    });
    read_records(entities.surfaces, surfaces, [&](SurfaceEntity& surface) {
        load_bounded_entity(in, surface, binary);
        load_tags(in, surface.boundary_curve_tags, binary);
    });
    read_records(entities.volumes, volumes, [&](VolumeEntity& volume) {
        load_bounded_entity(in, volume, binary);
        load_tags(in, volume.boundary_surface_tags, binary);
    });
}
} // namespace mshio
