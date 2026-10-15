#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mshio {

struct CurveParametrization
{
    std::int32_t tag = 0;
    std::uint64_t num_nodes = 0;
    // Four doubles per local node, in wire order: x, y, z, u.
    // These are discrete geometry samples, not references to the Nodes section.
    std::vector<double> data;
};

struct SurfaceParametrization
{
    std::int32_t tag = 0;
    std::uint64_t num_nodes = 0;
    std::uint64_t num_triangles = 0;
    // Eleven doubles per local node: x, y, z, u, v, curvMaxX, curvMaxY,
    // curvMaxZ, curvMinX, curvMinY, curvMinZ. Curvatures are vectors, not scalars.
    std::vector<double> data;
    // Three signed 32-bit, zero-based indices into this surface's local nodes
    // per triangle. They are neither global mesh node tags nor one-based IDs.
    std::vector<std::int32_t> triangle_indices;
};

struct Parametrizations
{
    std::vector<CurveParametrization> curves;
    std::vector<SurfaceParametrization> surfaces;

    bool empty() const { return curves.empty() && surfaces.empty(); }
};

struct InterpolationMatrix
{
    std::int32_t num_rows = 0;
    std::int32_t num_columns = 0;
    std::vector<double> data; // Row-major, num_rows * num_columns entries.
};

struct InterpolationTopology
{
    // MSH topology ID, not an MSH element type: 1 point, 2 line, 3 triangle,
    // 4 quadrangle, 5 tetrahedron, 6 pyramid, 7 prism, 8 hexahedron,
    // 9 polygon, 10 polyhedron.
    std::int32_t element_topology = 0;
    // Preserve every matrix in file order; Gmsh commonly writes two.
    std::vector<InterpolationMatrix> matrices;
};

struct InterpolationScheme
{
    std::string name;
    std::vector<InterpolationTopology> topologies;
    // Schemes must precede datasets referring to their name when writing MSH.
    // Their entire section is ASCII, even in a binary MSH 2.2 or 4.1 file.
};

} // namespace mshio
