#include "msh_extra_sections_io.h"
#include "io_utils.h"

#include <mshio/MshSpec.h>
#include <mshio/exception.h>

#include <utility>

namespace mshio {
namespace {

void require_supported_version(const MshSpec& spec)
{
    if (spec.mesh_format.version != "4.1" && spec.mesh_format.version != "2.2")
        throw InvalidFormat("Unsupported version " + spec.mesh_format.version);
}

void validate_curve(const CurveParametrization& curve)
{
    if (curve.tag <= 0) throw InvalidFormat("Invalid parametrized curve tag");
    const auto count = checked_product(curve.num_nodes, 4);
    checked_bytes(count, sizeof(double));
    if (curve.data.size() != count) throw InvalidFormat("Inconsistent curve parametrization size");
}

void validate_surface(const SurfaceParametrization& surface)
{
    if (surface.tag <= 0) throw InvalidFormat("Invalid parametrized surface tag");
    const auto nodes = checked_product(surface.num_nodes, 11);
    const auto indices = checked_product(surface.num_triangles, 3);
    checked_bytes(nodes, sizeof(double));
    checked_bytes(indices, sizeof(std::int32_t));
    if (surface.data.size() != nodes || surface.triangle_indices.size() != indices)
        throw InvalidFormat("Inconsistent surface parametrization size");
    for (auto index : surface.triangle_indices) {
        if (index < 0 || static_cast<std::uint64_t>(index) >= surface.num_nodes)
            throw InvalidFormat("Parametrization triangle index is out of bounds");
    }
}

void validate_scheme(const InterpolationScheme& scheme)
{
    if (scheme.name.find_first_of("\"\r\n") != std::string::npos)
        throw InvalidFormat("MSH quoted strings cannot contain quotes or line breaks");
    checked_int32(scheme.topologies.size());
    for (const auto& topology : scheme.topologies) {
        if (topology.element_topology < 1 || topology.element_topology > 10)
            throw InvalidFormat("Invalid interpolation element topology");
        checked_int32(topology.matrices.size());
        for (const auto& matrix : topology.matrices) {
            const auto count =
                checked_product(nonnegative(matrix.num_rows), nonnegative(matrix.num_columns));
            checked_bytes(count, sizeof(double));
            if (matrix.data.size() != count)
                throw InvalidFormat("Inconsistent interpolation matrix size");
        }
    }
}

// Do not let the outer section scanner silently discard numeric records that
// contradict the declared sizes. Leave the terminator for load_msh's scanner.
void require_section_boundary(std::istream& in)
{
    in >> std::ws;
    if (in.peek() != '$') throw InvalidFormat("Invalid MSH extra section boundary");
}

template <typename T>
void write_field(std::ostream& out, T value, bool binary)
{
    if (binary)
        write_binary(out, &value);
    else
        out << value << '\n';
}

template <typename T>
void write_values(
    std::ostream& out,
    const std::vector<T>& values,
    std::uint64_t stride,
    bool binary)
{
    if (binary) {
        write_binary(out, values.data(), values.size());
    } else {
        for (size_t i = 0; i < values.size(); ++i)
            out << values[i] << ((i + 1) % stride == 0 ? '\n' : ' ');
    }
}

} // namespace

void validate_extra_sections(const MshSpec& spec)
{
    if (spec.parametrizations.empty() && spec.interpolation_schemes.empty()) return;
    require_supported_version(spec);
    if (!spec.parametrizations.empty() && spec.mesh_format.version != "4.1")
        throw InvalidFormat("Parametrizations require MSH 4.1; cannot save them as MSH 2.2");
    checked_count(spec.parametrizations.curves, spec.parametrizations.curves.size());
    checked_count(spec.parametrizations.surfaces, spec.parametrizations.surfaces.size());
    for (const auto& curve : spec.parametrizations.curves) validate_curve(curve);
    for (const auto& surface : spec.parametrizations.surfaces) validate_surface(surface);
    for (const auto& scheme : spec.interpolation_schemes) validate_scheme(scheme);
}

void load_parametrizations(std::istream& in, MshSpec& spec)
{
    if (spec.mesh_format.version != "4.1") throw InvalidFormat("Parametrizations require MSH 4.1");
    const bool binary = spec.mesh_format.file_type > 0;
    if (binary) eat_white_space(in, 1);
    std::uint64_t num_curves, num_surfaces;
    read_field(in, num_curves, binary);
    read_field(in, num_surfaces, binary);
    checked_sum(num_curves, num_surfaces);
    auto& params = spec.parametrizations;
    read_records(params.curves, num_curves, [&](CurveParametrization& curve) {
        read_field(in, curve.tag, binary);
        read_field(in, curve.num_nodes, binary);
        read_values(in, curve.data, checked_product(curve.num_nodes, 4), binary);
        validate_curve(curve);
    });
    read_records(params.surfaces, num_surfaces, [&](SurfaceParametrization& surface) {
        read_field(in, surface.tag, binary);
        read_field(in, surface.num_nodes, binary);
        read_field(in, surface.num_triangles, binary);
        const auto nodes = checked_product(surface.num_nodes, 11);
        const auto triangles = checked_product(surface.num_triangles, 3);
        // Check both declarations before reading or allocating either payload.
        checked_bytes(nodes, sizeof(double));
        checked_bytes(triangles, sizeof(std::int32_t));
        read_values(in, surface.data, nodes, binary);
        read_values(in, surface.triangle_indices, triangles, binary);
        validate_surface(surface);
    });
    require_section_boundary(in);
}

void load_interpolation_scheme(std::istream& in, MshSpec& spec)
{
    require_supported_version(spec);
    InterpolationScheme scheme;
    read_quoted_string(in, scheme.name);
    std::int32_t count;
    read_ascii(in, count);
    read_records(scheme.topologies, nonnegative(count), [&](InterpolationTopology& topology) {
        read_ascii(in, topology.element_topology);
        std::int32_t matrices;
        read_ascii(in, matrices);
        read_records(topology.matrices, nonnegative(matrices), [&](InterpolationMatrix& matrix) {
            read_ascii(in, matrix.num_rows);
            read_ascii(in, matrix.num_columns);
            read_values(
                in,
                matrix.data,
                checked_product(nonnegative(matrix.num_rows), nonnegative(matrix.num_columns)),
                false);
        });
    });
    validate_scheme(scheme);
    require_section_boundary(in);
    auto& schemes = spec.interpolation_schemes;
    checked_count(schemes, checked_sum(schemes.size(), 1));
    schemes.emplace_back(std::move(scheme));
}

void save_parametrizations(std::ostream& out, const MshSpec& spec)
{
    const auto& params = spec.parametrizations;
    if (params.empty()) return;
    if (spec.mesh_format.version != "4.1")
        throw InvalidFormat("Parametrizations require MSH 4.1; cannot save them as MSH 2.2");
    for (const auto& curve : params.curves) validate_curve(curve);
    for (const auto& surface : params.surfaces) validate_surface(surface);
    const bool binary = spec.mesh_format.file_type > 0;
    out << "$Parametrizations\n";
    write_field(out, static_cast<std::uint64_t>(params.curves.size()), binary);
    write_field(out, static_cast<std::uint64_t>(params.surfaces.size()), binary);
    for (const auto& curve : params.curves) {
        write_field(out, curve.tag, binary);
        write_field(out, curve.num_nodes, binary);
        write_values(out, curve.data, 4, binary);
    }
    for (const auto& surface : params.surfaces) {
        write_field(out, surface.tag, binary);
        write_field(out, surface.num_nodes, binary);
        write_field(out, surface.num_triangles, binary);
        write_values(out, surface.data, 11, binary);
        write_values(out, surface.triangle_indices, 3, binary);
    }
    if (binary) out << '\n';
    out << "$EndParametrizations\n";
    if (!out) throw InvalidFormat("Failed to write MSH parametrizations");
}

void save_interpolation_schemes(std::ostream& out, const MshSpec& spec)
{
    if (spec.interpolation_schemes.empty()) return;
    require_supported_version(spec);
    for (const auto& scheme : spec.interpolation_schemes) validate_scheme(scheme);
    for (const auto& scheme : spec.interpolation_schemes) {
        out << "$InterpolationScheme\n";
        write_quoted_string(out, scheme.name);
        out << '\n' << scheme.topologies.size() << '\n';
        for (const auto& topology : scheme.topologies) {
            out << topology.element_topology << '\n' << topology.matrices.size() << '\n';
            for (const auto& matrix : topology.matrices) {
                out << matrix.num_rows << ' ' << matrix.num_columns << '\n';
                // No iteration for empty matrices, so a zero column count is safe.
                write_values(out, matrix.data, nonnegative(matrix.num_columns), false);
            }
        }
        out << "$EndInterpolationScheme\n";
    }
    if (!out) throw InvalidFormat("Failed to write MSH interpolation schemes");
}

} // namespace mshio
