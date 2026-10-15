#include "msh_topology_io.h"
#include <mshio/MshSpec.h>
#include "io_utils.h"

#include <cctype>
#include <string>

namespace mshio {
namespace {

// Wire layouts follow released Gmsh 4.15.2 GModelIO_MSH4.cpp and
// GModelIO_MSH3.cpp (the latter supplies MSH 2.2's ASCII Periodic section).
template <typename T>
void save_field(std::ostream& out, const T& value, bool binary)
{
    if (binary)
        write_binary(out, &value);
    else
        out << value << ' ';
}

void load_tags(std::istream& in, std::vector<std::int32_t>& tags, bool binary)
{
    std::uint64_t count;
    read_field(in, count, binary);
    read_values(in, tags, count, binary);
}

void save_tags(std::ostream& out, const std::vector<std::int32_t>& tags, bool binary)
{
    const std::uint64_t count = tags.size();
    save_field(out, count, binary);
    if (binary)
        write_binary(out, tags.data(), count);
    else
        for (auto tag : tags) save_field(out, tag, false);
}

template <typename Entity>
void load_partition_info(std::istream& in, Entity& entity, bool binary)
{
    read_field(in, entity.tag, binary);
    read_field(in, entity.parent_dim, binary);
    read_field(in, entity.parent_tag, binary);
    load_tags(in, entity.partitions, binary);
}

template <typename Entity>
void save_partition_info(std::ostream& out, const Entity& entity, bool binary)
{
    save_field(out, entity.tag, binary);
    save_field(out, entity.parent_dim, binary);
    save_field(out, entity.parent_tag, binary);
    save_tags(out, entity.partitions, binary);
}

template <typename Entity>
void load_bounded_entity(std::istream& in, Entity& entity, bool binary)
{
    load_partition_info(in, entity, binary);
    read_field(in, entity.min_x, binary);
    read_field(in, entity.min_y, binary);
    read_field(in, entity.min_z, binary);
    read_field(in, entity.max_x, binary);
    read_field(in, entity.max_y, binary);
    read_field(in, entity.max_z, binary);
    load_tags(in, entity.physical_group_tags, binary);
}

template <typename Entity>
void save_bounded_entity(std::ostream& out, const Entity& entity, bool binary)
{
    save_partition_info(out, entity, binary);
    save_field(out, entity.min_x, binary);
    save_field(out, entity.min_y, binary);
    save_field(out, entity.min_z, binary);
    save_field(out, entity.max_x, binary);
    save_field(out, entity.max_y, binary);
    save_field(out, entity.max_z, binary);
    save_tags(out, entity.physical_group_tags, binary);
}

void require(bool condition, const char* message)
{
    if (!condition) throw CorruptData(message);
}

void validate_partition(std::int32_t partition, std::uint64_t count)
{
    require(
        partition > 0 && static_cast<std::uint64_t>(partition) <= count,
        "Partitioned entity has invalid partition tag");
}

template <typename Entity>
void validate_partition_entities(const std::vector<Entity>& entities, std::uint64_t count)
{
    for (const auto& entity : entities) {
        require(entity.tag > 0, "Invalid partitioned entity tag");
        require(
            entity.parent_dim >= 0 && entity.parent_dim <= 3 && entity.parent_tag >= 0,
            "Invalid partitioned entity parent");
        for (auto partition : entity.partitions) validate_partition(partition, count);
    }
}

void validate_partitioned(const PartitionedEntities& entities)
{
    if (entities.empty()) return;
    require(entities.num_partitions > 0, "PartitionedEntities requires a partition count");
    for (const auto& ghost : entities.ghost_entities) {
        require(ghost.tag > 0, "Invalid ghost entity tag");
        validate_partition(ghost.partition, entities.num_partitions);
    }
    validate_partition_entities(entities.points, entities.num_partitions);
    validate_partition_entities(entities.curves, entities.num_partitions);
    validate_partition_entities(entities.surfaces, entities.num_partitions);
    validate_partition_entities(entities.volumes, entities.num_partitions);
}

void validate_periodic(const std::vector<PeriodicLink>& links, bool legacy)
{
    if (legacy) checked_int32(links.size());
    for (const auto& link : links) {
        require(link.entity_dim >= 0 && link.entity_dim <= 2, "Invalid periodic entity dimension");
        require(link.slave_tag > 0 && link.master_tag > 0, "Invalid periodic entity tag");
        require(
            link.affine_transform.empty() || link.affine_transform.size() == 16,
            "Periodic affine transformation must contain zero or sixteen values");
        checked_bytes(checked_product(link.node_pairs.size(), 2), sizeof(std::uint64_t));
        if (legacy) checked_int32(link.node_pairs.size());
        for (const auto& pair : link.node_pairs) {
            require(pair.slave_tag > 0 && pair.master_tag > 0, "Invalid periodic node tag");
            if (legacy) {
                checked_int32(pair.slave_tag);
                checked_int32(pair.master_tag);
            }
        }
    }
}

void validate_ghosts(const std::vector<GhostElement>& elements)
{
    for (const auto& element : elements) {
        require(element.element_tag > 0, "Invalid ghost element tag");
        require(element.partition > 0, "Invalid ghost element owning partition");
        for (auto partition : element.ghost_partitions)
            require(partition > 0, "Invalid ghost element receiving partition");
    }
}

void require_modern(const MshSpec& spec, const char* section)
{
    if (spec.mesh_format.version != "4.1")
        throw UnsupportedFeature(std::string(section) + " requires MSH version 4.1");
}

std::uint64_t load_legacy_count(std::istream& in)
{
    std::int32_t count;
    read_ascii(in, count);
    return nonnegative(count);
}

void finish_section(std::ostream& out, const char* name, bool binary)
{
    if (binary) out << '\n';
    out << "$End" << name << '\n';
    if (!out) throw InvalidFormat(std::string("Failed to write MSH ") + name);
}

} // namespace

void load_partitioned_entities(std::istream& in, MshSpec& spec)
{
    require_modern(spec, "$PartitionedEntities");
    const bool binary = spec.mesh_format.file_type == 1;
    if (binary) eat_white_space(in, 1);
    auto& entities = spec.partitioned_entities;
    read_field(in, entities.num_partitions, binary);
    std::uint64_t ghost_count, points, curves, surfaces, volumes;
    read_field(in, ghost_count, binary);
    checked_bytes(checked_product(ghost_count, 2), sizeof(std::int32_t));
    read_records(entities.ghost_entities, ghost_count, [&](GhostEntity& ghost) {
        read_field(in, ghost.tag, binary);
        read_field(in, ghost.partition, binary);
    });
    read_field(in, points, binary);
    read_field(in, curves, binary);
    read_field(in, surfaces, binary);
    read_field(in, volumes, binary);
    checked_sum(checked_sum(points, curves), checked_sum(surfaces, volumes));
    read_records(entities.points, points, [&](PartitionedPointEntity& point) {
        load_partition_info(in, point, binary);
        read_field(in, point.x, binary);
        read_field(in, point.y, binary);
        read_field(in, point.z, binary);
        load_tags(in, point.physical_group_tags, binary);
    });
    read_records(entities.curves, curves, [&](PartitionedCurveEntity& curve) {
        load_bounded_entity(in, curve, binary);
        load_tags(in, curve.boundary_point_tags, binary);
    });
    read_records(entities.surfaces, surfaces, [&](PartitionedSurfaceEntity& surface) {
        load_bounded_entity(in, surface, binary);
        load_tags(in, surface.boundary_curve_tags, binary);
    });
    read_records(entities.volumes, volumes, [&](PartitionedVolumeEntity& volume) {
        load_bounded_entity(in, volume, binary);
        load_tags(in, volume.boundary_surface_tags, binary);
    });
    validate_partitioned(entities);
}

void load_periodic(std::istream& in, MshSpec& spec)
{
    const bool legacy = spec.mesh_format.version == "2.2";
    if (!legacy) require_modern(spec, "$Periodic");
    // The legacy section is always text, including in a binary MSH 2.2 mesh.
    const bool binary = !legacy && spec.mesh_format.file_type == 1;
    if (binary) eat_white_space(in, 1);
    std::uint64_t count;
    if (legacy)
        count = load_legacy_count(in);
    else
        read_field(in, count, binary);
    read_records(spec.periodic_links, count, [&](PeriodicLink& link) {
        read_field(in, link.entity_dim, binary);
        read_field(in, link.slave_tag, binary);
        read_field(in, link.master_tag, binary);
        std::uint64_t affine_count = 0;
        if (legacy) {
            in >> std::ws;
            if (in.peek() == 'A') {
                constexpr char marker[] = "Affine";
                for (size_t i = 0; i + 1 < sizeof(marker); ++i)
                    if (in.get() != marker[i])
                        throw InvalidFormat("Invalid periodic Affine marker");
                const auto ch = in.peek();
                if (ch == std::char_traits<char>::eof() ||
                    !std::isspace(static_cast<unsigned char>(ch)))
                    throw InvalidFormat("Invalid periodic Affine marker");
                affine_count = 16;
            }
        } else {
            read_field(in, affine_count, binary);
        }
        if (affine_count != 0 && affine_count != 16)
            throw InvalidFormat(
                "Periodic affine transformation must contain zero or sixteen values");
        read_values(in, link.affine_transform, affine_count, binary);
        std::uint64_t pairs;
        if (legacy)
            pairs = load_legacy_count(in);
        else
            read_field(in, pairs, binary);
        checked_bytes(checked_product(pairs, 2), sizeof(std::uint64_t));
        read_records(link.node_pairs, pairs, [&](PeriodicNodePair& pair) {
            if (legacy) {
                std::int32_t slave, master;
                read_ascii(in, slave);
                read_ascii(in, master);
                pair.slave_tag = nonnegative(slave);
                pair.master_tag = nonnegative(master);
            } else {
                read_field(in, pair.slave_tag, binary);
                read_field(in, pair.master_tag, binary);
            }
        });
    });
    validate_periodic(spec.periodic_links, legacy);
}

void load_ghost_elements(std::istream& in, MshSpec& spec)
{
    require_modern(spec, "$GhostElements");
    const bool binary = spec.mesh_format.file_type == 1;
    if (binary) eat_white_space(in, 1);
    std::uint64_t count;
    read_field(in, count, binary);
    read_records(spec.ghost_elements, count, [&](GhostElement& element) {
        read_field(in, element.element_tag, binary);
        read_field(in, element.partition, binary);
        load_tags(in, element.ghost_partitions, binary);
    });
    validate_ghosts(spec.ghost_elements);
}

void save_partitioned_entities(std::ostream& out, const MshSpec& spec)
{
    const auto& entities = spec.partitioned_entities;
    if (entities.empty()) return;
    require_modern(spec, "$PartitionedEntities");
    validate_partitioned(entities);
    const bool binary = spec.mesh_format.file_type == 1;
    out << "$PartitionedEntities\n";
    save_field(out, entities.num_partitions, binary);
    if (!binary) out << '\n';
    save_field(out, std::uint64_t(entities.ghost_entities.size()), binary);
    if (!binary) out << '\n';
    for (const auto& ghost : entities.ghost_entities) {
        save_field(out, ghost.tag, binary);
        save_field(out, ghost.partition, binary);
        if (!binary) out << '\n';
    }
    save_field(out, std::uint64_t(entities.points.size()), binary);
    save_field(out, std::uint64_t(entities.curves.size()), binary);
    save_field(out, std::uint64_t(entities.surfaces.size()), binary);
    save_field(out, std::uint64_t(entities.volumes.size()), binary);
    if (!binary) out << '\n';
    for (const auto& point : entities.points) {
        save_partition_info(out, point, binary);
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
    finish_section(out, "PartitionedEntities", binary);
}

void save_periodic(std::ostream& out, const MshSpec& spec)
{
    if (spec.periodic_links.empty()) return;
    const bool legacy = spec.mesh_format.version == "2.2";
    if (!legacy) require_modern(spec, "$Periodic");
    validate_periodic(spec.periodic_links, legacy);
    const bool binary = !legacy && spec.mesh_format.file_type == 1;
    out << "$Periodic\n";
    save_field(out, std::uint64_t(spec.periodic_links.size()), binary);
    if (!binary) out << '\n';
    for (const auto& link : spec.periodic_links) {
        save_field(out, link.entity_dim, binary);
        save_field(out, link.slave_tag, binary);
        save_field(out, link.master_tag, binary);
        if (!binary) out << '\n';
        if (legacy) {
            if (!link.affine_transform.empty()) out << "Affine ";
        } else {
            save_field(out, std::uint64_t(link.affine_transform.size()), binary);
        }
        if (binary)
            write_binary(out, link.affine_transform.data(), link.affine_transform.size());
        else
            for (auto value : link.affine_transform) save_field(out, value, false);
        if (!binary && (!legacy || !link.affine_transform.empty())) out << '\n';
        save_field(out, std::uint64_t(link.node_pairs.size()), binary);
        if (!binary) out << '\n';
        for (const auto& pair : link.node_pairs) {
            save_field(out, pair.slave_tag, binary);
            save_field(out, pair.master_tag, binary);
            if (!binary) out << '\n';
        }
    }
    finish_section(out, "Periodic", binary);
}

void save_ghost_elements(std::ostream& out, const MshSpec& spec)
{
    if (spec.ghost_elements.empty()) return;
    require_modern(spec, "$GhostElements");
    validate_ghosts(spec.ghost_elements);
    const bool binary = spec.mesh_format.file_type == 1;
    out << "$GhostElements\n";
    save_field(out, std::uint64_t(spec.ghost_elements.size()), binary);
    if (!binary) out << '\n';
    for (const auto& element : spec.ghost_elements) {
        save_field(out, element.element_tag, binary);
        save_field(out, element.partition, binary);
        save_tags(out, element.ghost_partitions, binary);
        if (!binary) out << '\n';
    }
    finish_section(out, "GhostElements", binary);
}

void validate_topology_sections(const MshSpec& spec)
{
    if (!spec.partitioned_entities.empty() || !spec.ghost_elements.empty())
        require_modern(spec, "PartitionedEntities and GhostElements");
    validate_partitioned(spec.partitioned_entities);
    validate_periodic(spec.periodic_links, spec.mesh_format.version == "2.2");
    validate_ghosts(spec.ghost_elements);
}

} // namespace mshio
