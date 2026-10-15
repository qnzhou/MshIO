"""Full-field bindings and independent Gmsh geometry/interpolation checks."""

from pathlib import Path
import tempfile
import unittest

import numpy as np
import mshio

try:
    import gmsh
except ModuleNotFoundError as error:
    if error.name != "gmsh":
        raise
    gmsh = None


def snapshot(spec):
    return (
        [(curve.tag, curve.num_nodes, curve.data)
         for curve in spec.parametrizations.curves],
        [(surface.tag, surface.num_nodes, surface.num_triangles, surface.data,
          surface.triangle_indices) for surface in spec.parametrizations.surfaces],
        [(scheme.name,
          [(topology.element_topology,
            [(matrix.num_rows, matrix.num_columns, matrix.data)
             for matrix in topology.matrices]) for topology in scheme.topologies])
         for scheme in spec.interpolation_schemes],
    )


def make_spec():
    spec = mshio.MshSpec()
    curve = mshio.CurveParametrization()
    curve.tag = 12
    curve.num_nodes = 2
    curve.data = [0., 0., 0., -1., 2., 0., 0., 3.]
    spec.parametrizations.curves.append(curve)
    surface = mshio.SurfaceParametrization()
    surface.tag = 31
    surface.num_nodes = 3
    surface.num_triangles = 1
    surface.data = [
        0., 0., 0., 0., 0., 1., 2., 3., 4., 5., 6.,
        2., 0., 0., 1., 0., 7., 8., 9., 10., 11., 12.,
        0., 2., 0., 0., 1., 13., 14., 15., 16., 17., 18.,
    ]
    surface.triangle_indices = [0, 2, 1]
    spec.parametrizations.surfaces.append(surface)
    for name in (r"basis with \literal\ paths", "second basis"):
        scheme = mshio.InterpolationScheme()
        scheme.name = name
        topology = mshio.InterpolationTopology()
        topology.element_topology = 3
        for rows, columns, values in (
            (2, 3, [1., 0., 0., -1., 2., .5]),
            (3, 2, [0., 0., 1., 0., 0., 1.]),
            (1, 2, [2., 3.]),
        ):
            matrix = mshio.InterpolationMatrix()
            matrix.num_rows = rows
            matrix.num_columns = columns
            matrix.data = values
            topology.matrices.append(matrix)
        scheme.topologies.append(topology)
        spec.interpolation_schemes.append(scheme)
    data = mshio.Data()
    data.header.string_tags = ["samples", spec.interpolation_schemes[0].name]
    data.header.real_tags = [.25]
    data.header.int_tags = [0, 1, 1]
    entry = mshio.DataEntry()
    entry.tag = 93
    entry.num_nodes_per_elements = 2
    entry.data = [2., 7.]
    data.entries.append(entry)
    spec.element_node_data.append(data)
    return spec


class ExtraSectionBindingTests(unittest.TestCase):
    def test_all_fields_and_data_only_files(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "extras.msh"
            for version in ("2.2", "4.1"):
                for binary in (0, 1):
                    with self.subTest(version=version, binary=binary):
                        spec = make_spec()
                        if version == "2.2":
                            spec.parametrizations.curves.clear()
                            spec.parametrizations.surfaces.clear()
                        spec.mesh_format.version = version
                        spec.mesh_format.file_type = binary
                        expected = snapshot(spec)
                        mshio.save_msh(str(path), spec)
                        loaded = mshio.load_msh(str(path))
                        self.assertEqual(snapshot(loaded), expected)
                        self.assertEqual(len(loaded.nodes.entity_blocks), 0)
                        self.assertEqual(len(loaded.elements.entity_blocks), 0)
                        self.assertEqual(loaded.element_node_data[0].header.string_tags,
                                         spec.element_node_data[0].header.string_tags)
                        np.testing.assert_array_equal(
                            loaded.element_node_data[0].entries[0].data, [2., 7.])
                        raw = path.read_bytes()
                        self.assertLess(raw.index(b"$InterpolationScheme"),
                                        raw.index(b"$ElementNodeData"))
                        for section in raw.split(b"$InterpolationScheme\n")[1:]:
                            section.split(b"$EndInterpolationScheme", 1)[0].decode("ascii")

    def test_object_vectors_assign_back_and_numeric_vectors_are_lists(self):
        spec = make_spec()
        curve = spec.parametrizations.curves[0]
        surface = spec.parametrizations.surfaces[0]
        scheme = spec.interpolation_schemes[0]
        topology = scheme.topologies[0]
        matrix = topology.matrices[0]
        curve.tag = 15
        surface.tag = 35
        scheme.name = "changed"
        topology.element_topology = 2
        matrix.num_rows = 3
        matrix.num_columns = 2
        self.assertEqual(spec.parametrizations.curves[0].tag, 12)
        self.assertEqual(spec.parametrizations.surfaces[0].tag, 31)
        self.assertNotEqual(spec.interpolation_schemes[0].name, "changed")
        self.assertEqual(spec.interpolation_schemes[0].topologies[0].element_topology, 3)
        self.assertEqual(spec.interpolation_schemes[0].topologies[0].matrices[0].num_rows, 2)
        topology.matrices[0] = matrix
        scheme.topologies[0] = topology
        spec.interpolation_schemes[0] = scheme
        spec.parametrizations.curves[0] = curve
        spec.parametrizations.surfaces[0] = surface
        self.assertEqual(spec.parametrizations.curves[0].tag, 15)
        self.assertEqual(spec.parametrizations.surfaces[0].tag, 35)
        self.assertEqual(spec.interpolation_schemes[0].name, "changed")
        self.assertEqual(spec.interpolation_schemes[0].topologies[0].element_topology, 2)
        self.assertEqual(spec.interpolation_schemes[0].topologies[0].matrices[0].num_rows, 3)
        for record, field in ((curve, "data"), (surface, "data"),
                              (surface, "triangle_indices"), (matrix, "data")):
            original = getattr(record, field)
            self.assertIsInstance(original, list)
            changed = list(original)
            changed[0] += 1
            original[0] += 10
            self.assertNotEqual(original, getattr(record, field))
            setattr(record, field, changed)
            self.assertEqual(getattr(record, field), changed)
        topology.matrices[0] = matrix
        scheme.topologies[0] = topology
        spec.interpolation_schemes[0] = scheme
        spec.parametrizations.curves[0] = curve
        spec.parametrizations.surfaces[0] = surface
        self.assertEqual(spec.parametrizations.curves[0].data, curve.data)
        self.assertEqual(spec.parametrizations.surfaces[0].data, surface.data)
        self.assertEqual(spec.parametrizations.surfaces[0].triangle_indices,
                         surface.triangle_indices)
        self.assertEqual(spec.interpolation_schemes[0].topologies[0].matrices[0].data,
                         matrix.data)

    def test_invalid_shapes_and_legacy_downgrade_raise(self):
        with tempfile.TemporaryDirectory() as directory:
            path = str(Path(directory) / "bad.msh")
            spec = make_spec()
            spec.mesh_format.version = "2.2"
            with self.assertRaises(Exception):
                mshio.save_msh(path, spec)
            for field, value in (("num_nodes", 4), ("num_triangles", 2),
                                 ("triangle_indices", [-1, 1, 2]),
                                 ("triangle_indices", [0, 1, 3])):
                spec = make_spec()
                surface = spec.parametrizations.surfaces[0]
                setattr(surface, field, value)
                spec.parametrizations.surfaces[0] = surface
                with self.assertRaises(Exception):
                    mshio.save_msh(path, spec)
            spec = make_spec()
            scheme = spec.interpolation_schemes[0]
            topology = scheme.topologies[0]
            matrix = topology.matrices[0]
            matrix.data = [1.]
            topology.matrices[0] = matrix
            scheme.topologies[0] = topology
            spec.interpolation_schemes[0] = scheme
            with self.assertRaises(Exception):
                mshio.save_msh(path, spec)


@unittest.skipIf(gmsh is None, "Gmsh oracle checks require pip install '.[test]'")
class GmshExtraSectionTests(unittest.TestCase):
    def setUp(self):
        gmsh.initialize([], readConfigFiles=False)
        self.addCleanup(gmsh.finalize)
        gmsh.option.setNumber("General.Terminal", 0)

    def make_parametrized_square(self):
        gmsh.model.add("discrete square")
        for tag in range(1, 5):
            gmsh.model.addDiscreteEntity(0, tag)
        for tag, boundary in enumerate(((1, 2), (2, 3), (3, 4), (4, 1)), 1):
            gmsh.model.addDiscreteEntity(1, tag, list(boundary))
        gmsh.model.addDiscreteEntity(2, 1, [1, 2, 3, 4])
        gmsh.model.mesh.addNodes(2, 1, [1, 2, 3, 4],
                                 [0., 0., 0., 1., 0., 0., 1., 1., 0., 0., 1., 0.])
        for tag in range(1, 5):
            gmsh.model.mesh.addElementsByType(tag, 15, [tag], [tag])
        for tag, boundary in enumerate(((1, 2), (2, 3), (3, 4), (4, 1)), 1):
            gmsh.model.mesh.addElementsByType(tag, 1, [tag + 4], list(boundary))
        gmsh.model.mesh.addElementsByType(1, 2, [9, 10], [1, 2, 3, 1, 3, 4])
        gmsh.model.mesh.reclassifyNodes()
        gmsh.model.mesh.createGeometry()
        gmsh.option.setNumber("Mesh.SaveAll", 1)
        gmsh.option.setNumber("Mesh.SaveParametric", 1)
        gmsh.option.setNumber("Mesh.MshFileVersion", 4.1)

    def test_gmsh_curve_surface_parametrizations_both_directions(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "gmsh.msh"
            target = Path(directory) / "mshio.msh"
            for binary in (0, 1):
                with self.subTest(binary=binary):
                    gmsh.clear()
                    self.make_parametrized_square()
                    gmsh.option.setNumber("Mesh.Binary", binary)
                    gmsh.write(str(source))
                    spec = mshio.load_msh(str(source))
                    self.assertEqual(len(spec.parametrizations.curves), 4)
                    self.assertEqual(len(spec.parametrizations.surfaces), 1)
                    for curve in spec.parametrizations.curves:
                        self.assertEqual(len(curve.data), 4 * curve.num_nodes)
                        self.assertGreaterEqual(curve.num_nodes, 2)
                    surface = spec.parametrizations.surfaces[0]
                    self.assertEqual(len(surface.data), 11 * surface.num_nodes)
                    self.assertEqual(len(surface.triangle_indices), 3 * surface.num_triangles)
                    self.assertEqual(min(surface.triangle_indices), 0)
                    self.assertLess(max(surface.triangle_indices), surface.num_nodes)
                    expected = snapshot(spec)
                    expected_uv = gmsh.model.getParametrization(2, 1, [.5, .5, 0.])
                    expected_xyz = gmsh.model.getValue(2, 1, expected_uv)
                    mshio.save_msh(str(target), spec)
                    self.assertEqual(snapshot(mshio.load_msh(str(target))), expected)
                    gmsh.clear()
                    gmsh.open(str(target))
                    actual_uv = gmsh.model.getParametrization(2, 1, [.5, .5, 0.])
                    np.testing.assert_allclose(actual_uv, expected_uv, rtol=0, atol=1e-14)
                    np.testing.assert_allclose(gmsh.model.getValue(2, 1, actual_uv),
                                               expected_xyz, rtol=0, atol=1e-14)
                    for tag in range(1, 5):
                        np.testing.assert_allclose(
                            gmsh.model.getValue(1, tag, [0., 1.]),
                            np.asarray(spec.parametrizations.curves[tag - 1].data)
                            .reshape(-1, 4)[:, :3].ravel(), rtol=0, atol=1e-14)

    def test_gmsh_preserves_all_six_curvature_vector_components(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "geometry.msh"
            target = Path(directory) / "curvature.msh"
            oracle = Path(directory) / "rewritten.msh"
            for binary in (0, 1):
                with self.subTest(binary=binary):
                    gmsh.clear()
                    self.make_parametrized_square()
                    gmsh.option.setNumber("Mesh.Binary", binary)
                    gmsh.write(str(source))
                    spec = mshio.load_msh(str(source))
                    surface = spec.parametrizations.surfaces[0]
                    records = surface.data
                    for node in range(surface.num_nodes):
                        records[node * 11 + 5:node * 11 + 11] = [
                            float(node * 6 + component + 1) for component in range(6)]
                    surface.data = records
                    spec.parametrizations.surfaces[0] = surface
                    mshio.save_msh(str(target), spec)
                    gmsh.clear()
                    gmsh.open(str(target))
                    gmsh.write(str(oracle))
                    self.assertEqual(snapshot(mshio.load_msh(str(oracle))), snapshot(spec))

    def test_gmsh_interpolation_matrices_and_interpolated_data(self):
        coefficients = [.25, -.25, -.25, .25, .25, .25, -.25, -.25,
                        .25, .25, .25, .25, .25, -.25, .25, -.25]
        exponents = [0., 0., 0., 1., 0., 0., 0., 1., 0., 1., 1., 0.]
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "gmsh.msh"
            target = Path(directory) / "mshio.msh"
            for version in ("2.2", "4.1"):
                for binary in (0, 1):
                    with self.subTest(version=version, binary=binary):
                        gmsh.clear()
                        gmsh.model.add("interpolation")
                        gmsh.model.addDiscreteEntity(2, 1)
                        gmsh.model.mesh.addNodes(
                            2, 1, [1, 2, 3, 4],
                            [0., 0., 0., 1., 0., 0., 1., 1., 0., 0., 1., 0.])
                        gmsh.model.mesh.addElementsByType(1, 3, [1], [1, 2, 3, 4])
                        view = gmsh.view.add("custom basis")
                        gmsh.view.addModelData(view, 0, "interpolation", "ElementNodeData",
                                               [1], [[1., 2., 3., 4.]], .25, 1)
                        gmsh.view.setInterpolationMatrices(view, "Quadrangle", 4,
                                                           coefficients, exponents)
                        gmsh.option.setNumber("Mesh.MshFileVersion", float(version))
                        gmsh.option.setNumber("Mesh.Binary", binary)
                        gmsh.option.setNumber("Mesh.SaveAll", 1)
                        gmsh.option.setNumber("PostProcessing.SaveMesh", 0)
                        gmsh.write(str(source))
                        gmsh.view.write(view, str(source), append=True)
                        spec = mshio.load_msh(str(source))
                        self.assertEqual(len(spec.interpolation_schemes), 1)
                        scheme = spec.interpolation_schemes[0]
                        self.assertEqual(scheme.name, "INTERPOLATION_SCHEME")
                        self.assertEqual(len(scheme.topologies), 1)
                        topology = scheme.topologies[0]
                        self.assertEqual(topology.element_topology, 4)
                        self.assertEqual(len(topology.matrices), 2)
                        first, second = topology.matrices
                        self.assertEqual((first.num_rows, first.num_columns), (4, 4))
                        self.assertEqual(first.data, coefficients)
                        self.assertEqual((second.num_rows, second.num_columns), (4, 3))
                        self.assertEqual(second.data, exponents)
                        raw = source.read_bytes().split(b"$InterpolationScheme\n", 1)[1]
                        raw.split(b"$EndInterpolationScheme", 1)[0].decode("ascii")
                        expected_data = spec.element_node_data[0].entries[0].data.copy()
                        expected_probe = gmsh.view.probe(view, .25, .25, 0.)[0]
                        np.testing.assert_allclose(expected_probe, [1.875], rtol=0, atol=1e-14)
                        mshio.save_msh(str(target), spec)
                        self.assertEqual(snapshot(mshio.load_msh(str(target))), snapshot(spec))
                        gmsh.clear()
                        gmsh.open(str(target))
                        views = gmsh.view.getTags()
                        self.assertEqual(len(views), 1)
                        actual = int(views[0])
                        np.testing.assert_allclose(gmsh.view.probe(actual, .25, .25, 0.)[0],
                                                   expected_probe, rtol=0, atol=1e-14)
                        kind, tags, values, time, components = gmsh.view.getModelData(actual, 0)
                        self.assertEqual(kind, "ElementNodeData")
                        np.testing.assert_array_equal(tags, [1])
                        np.testing.assert_array_equal(values[0], expected_data)
                        self.assertEqual(time, .25)
                        self.assertEqual(components, 1)


if __name__ == "__main__":
    unittest.main()
