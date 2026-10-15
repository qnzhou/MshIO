#pragma once

#include <cstdint>
#include <vector>

namespace mshio {

// Ghost entity metadata uses signed 32-bit entity and partition tags, not mesh tags.
struct GhostEntity
{
    std::int32_t tag = 0;
    std::int32_t partition = 0;
};

struct PartitionedPointEntity
{
    std::int32_t tag = 0;
    std::int32_t parent_dim = 0;
    std::int32_t parent_tag = 0;
    std::vector<std::int32_t> partitions;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
};

struct PartitionedCurveEntity
{
    std::int32_t tag = 0;
    std::int32_t parent_dim = 0;
    std::int32_t parent_tag = 0;
    std::vector<std::int32_t> partitions;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_point_tags;
};

struct PartitionedSurfaceEntity
{
    std::int32_t tag = 0;
    std::int32_t parent_dim = 0;
    std::int32_t parent_tag = 0;
    std::vector<std::int32_t> partitions;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_curve_tags;
};

struct PartitionedVolumeEntity
{
    std::int32_t tag = 0;
    std::int32_t parent_dim = 0;
    std::int32_t parent_tag = 0;
    std::vector<std::int32_t> partitions;
    double min_x = 0.0;
    double min_y = 0.0;
    double min_z = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double max_z = 0.0;
    std::vector<std::int32_t> physical_group_tags;
    std::vector<std::int32_t> boundary_surface_tags;
};

struct PartitionedEntities
{
    std::uint64_t num_partitions = 0;
    std::vector<GhostEntity> ghost_entities;
    std::vector<PartitionedPointEntity> points;
    std::vector<PartitionedCurveEntity> curves;
    std::vector<PartitionedSurfaceEntity> surfaces;
    std::vector<PartitionedVolumeEntity> volumes;

    bool empty() const
    {
        return num_partitions == 0 && ghost_entities.empty() && points.empty() && curves.empty() &&
               surfaces.empty() && volumes.empty();
    }
};

struct PeriodicNodePair
{
    std::uint64_t slave_tag = 0;
    std::uint64_t master_tag = 0;
};

struct PeriodicLink
{
    std::int32_t entity_dim = 0;
    std::int32_t slave_tag = 0;
    std::int32_t master_tag = 0;
    // Empty (no affine transformation) or a row-major 4-by-4 transformation.
    std::vector<double> affine_transform;
    std::vector<PeriodicNodePair> node_pairs;
};

struct GhostElement
{
    std::uint64_t element_tag = 0;
    std::int32_t partition = 0;
    std::vector<std::int32_t> ghost_partitions;
};

} // namespace mshio
