#pragma once

#include <mshio/MshSpecExtraSections.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

NB_MAKE_OPAQUE(std::vector<mshio::CurveParametrization>);
NB_MAKE_OPAQUE(std::vector<mshio::SurfaceParametrization>);
NB_MAKE_OPAQUE(std::vector<mshio::InterpolationMatrix>);
NB_MAKE_OPAQUE(std::vector<mshio::InterpolationTopology>);
NB_MAKE_OPAQUE(std::vector<mshio::InterpolationScheme>);

inline void bind_msh_extra_sections(nanobind::module_& m)
{
    namespace nb = nanobind;
    nb::class_<mshio::CurveParametrization>(m, "CurveParametrization")
        .def(nb::init<>())
        .def_rw("tag", &mshio::CurveParametrization::tag)
        .def_rw("num_nodes", &mshio::CurveParametrization::num_nodes)
        .def_rw("data", &mshio::CurveParametrization::data,
            "List of x, y, z, u records for local discrete-curve samples");
    nb::bind_vector<std::vector<mshio::CurveParametrization>>(m, "CurveParametrizations");

    nb::class_<mshio::SurfaceParametrization>(m, "SurfaceParametrization")
        .def(nb::init<>())
        .def_rw("tag", &mshio::SurfaceParametrization::tag)
        .def_rw("num_nodes", &mshio::SurfaceParametrization::num_nodes)
        .def_rw("num_triangles", &mshio::SurfaceParametrization::num_triangles)
        .def_rw("data", &mshio::SurfaceParametrization::data,
            "List of x,y,z,u,v,curvMaxXYZ,curvMinXYZ records (11 doubles per sample)")
        .def_rw("triangle_indices", &mshio::SurfaceParametrization::triangle_indices,
            "List of zero-based local sample indices, three per triangle");
    nb::bind_vector<std::vector<mshio::SurfaceParametrization>>(m, "SurfaceParametrizations");

    nb::class_<mshio::Parametrizations>(m, "Parametrizations")
        .def(nb::init<>())
        .def_rw("curves", &mshio::Parametrizations::curves)
        .def_rw("surfaces", &mshio::Parametrizations::surfaces)
        .def("empty", &mshio::Parametrizations::empty);

    nb::class_<mshio::InterpolationMatrix>(m, "InterpolationMatrix")
        .def(nb::init<>())
        .def_rw("num_rows", &mshio::InterpolationMatrix::num_rows)
        .def_rw("num_columns", &mshio::InterpolationMatrix::num_columns)
        .def_rw("data", &mshio::InterpolationMatrix::data, "Row-major numeric list");
    nb::bind_vector<std::vector<mshio::InterpolationMatrix>>(m, "InterpolationMatrices");

    nb::class_<mshio::InterpolationTopology>(m, "InterpolationTopology")
        .def(nb::init<>())
        .def_rw("element_topology", &mshio::InterpolationTopology::element_topology)
        .def_rw("matrices", &mshio::InterpolationTopology::matrices);
    nb::bind_vector<std::vector<mshio::InterpolationTopology>>(m, "InterpolationTopologies");

    nb::class_<mshio::InterpolationScheme>(m, "InterpolationScheme")
        .def(nb::init<>())
        .def_rw("name", &mshio::InterpolationScheme::name)
        .def_rw("topologies", &mshio::InterpolationScheme::topologies);
    nb::bind_vector<std::vector<mshio::InterpolationScheme>>(m, "InterpolationSchemes");
}
