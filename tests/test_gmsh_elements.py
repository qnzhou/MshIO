"""Element wire interoperability with the released Gmsh native writer/loader."""

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


# (MSH type, node count, dimension). Exercise complete/incomplete families and
# their upper boundaries without asking Gmsh to construct unsupported bases.
# Some valid MSH wire types have no released Gmsh factory branch; those are
# covered exhaustively by the native MshIO roundtrip tests, not silently skipped
# after a native Gmsh failure here.
NATIVE_TYPES = (
    (32, 22, 3), (33, 28, 3),
    (36, 16, 2), (39, 12, 2), (42, 28, 2), (46, 66, 2),
    (51, 121, 2), (52, 18, 2), (56, 30, 2), (61, 40, 2),
    (62, 7, 1), (66, 11, 1),
    (67, 2, 1), (68, 3, 2), (70, 2, 1),
    (71, 84, 3), (75, 286, 3), (79, 34, 3), (83, 58, 3),
    (90, 40, 3), (91, 75, 3), (92, 64, 3), (98, 1000, 3), (99, 32, 3),
    (106, 126, 3), (110, 550, 3), (111, 24, 3), (117, 78, 3),
    (118, 30, 3), (124, 385, 3), (137, 16, 3),
)


class ElementMetadataTests(unittest.TestCase):
    def test_new_type_metadata_is_exported_to_python(self):
        for element_type, count, dim in NATIVE_TYPES + (
            (84, 1, 1), (85, 1, 2), (86, 1, 2), (87, 1, 3),
            (88, 1, 3), (89, 1, 3), (105, 104, 3), (125, 21, 3),
            (131, 69, 3), (132, 1, 3), (133, 1, 0), (134, 2, 1),
            (135, 3, 2), (136, 4, 3), (140, 4, 3),
        ):
            with self.subTest(element_type=element_type):
                self.assertEqual(mshio.nodes_per_element(element_type), count)
                self.assertEqual(mshio.get_element_dim(element_type), dim)

    def test_non_fixed_width_and_undefined_types_are_rejected(self):
        for element_type in (-2**31, -1, 0, 34, 35, 69, 76, 77, 78,
                             138, 139, 141, 2**31 - 1):
            with self.subTest(element_type=element_type):
                with self.assertRaises(RuntimeError):
                    mshio.nodes_per_element(element_type)
                with self.assertRaises(RuntimeError):
                    mshio.get_element_dim(element_type)


@unittest.skipIf(gmsh is None, "Gmsh element checks require pip install '.[test]'")
class GmshElementTests(unittest.TestCase):
    def setUp(self):
        gmsh.initialize([], readConfigFiles=False)
        self.addCleanup(gmsh.finalize)
        gmsh.option.setNumber("General.Terminal", 0)
        gmsh.option.setNumber("Mesh.SaveAll", 1)

    def assert_native_roundtrip(self, directory, element_type, count, dim, versions):
        # Distinct coordinates/connectivity make omissions, stride mistakes and
        # element-boundary shifts observable. These are IO fixtures, not meshes
        # on which a shape function or geometric-quality algorithm is evaluated.
        tags = np.arange(1, count + 1, dtype=np.uint64)
        xyz = np.column_stack((tags / 13.0, (tags % 17) / 17.0,
                               (tags % 19) / 19.0))
        connectivity = np.concatenate((tags, np.roll(tags, 1)))
        records = np.column_stack((np.array([1, 2], dtype=np.uint64),
                                   connectivity.reshape(2, count))).ravel()
        for version in versions:
            for binary in (0, 1):
                with self.subTest(element_type=element_type, version=version,
                                  binary=binary):
                    gmsh.clear()
                    gmsh.model.add("element-oracle")
                    gmsh.model.addDiscreteEntity(dim, 1)
                    gmsh.model.mesh.addNodes(dim, 1, tags, xyz.ravel())
                    gmsh.model.mesh.addElementsByType(
                        1, element_type, [1, 2], connectivity
                    )
                    gmsh.option.setNumber("Mesh.MshFileVersion", float(version))
                    gmsh.option.setNumber("Mesh.Binary", binary)
                    source = Path(directory) / "gmsh-native.msh"
                    gmsh.write(str(source))

                    spec = mshio.load_msh(str(source))
                    mshio.validate_spec(spec)
                    self.assertEqual(spec.mesh_format.version, version)
                    self.assertEqual(spec.mesh_format.file_type, binary)
                    self.assertEqual(spec.elements.num_elements, 2)
                    self.assertEqual(len(spec.elements.entity_blocks), 1)
                    block = spec.elements.entity_blocks[0]
                    self.assertEqual(block.element_type, element_type)
                    self.assertEqual(block.entity_dim, dim)
                    self.assertEqual(block.entity_tag, 1)
                    self.assertEqual(block.num_elements_in_block, 2)
                    np.testing.assert_array_equal(block.data, records)
                    actual_nodes = {
                        int(tag): tuple(point)
                        for node_block in spec.nodes.entity_blocks
                        for tag, point in zip(node_block.tags,
                                              node_block.data.reshape(-1, 3))
                    }
                    self.assertEqual(set(actual_nodes), set(map(int, tags)))
                    np.testing.assert_allclose(
                        [actual_nodes[int(tag)] for tag in tags], xyz,
                        rtol=2e-15, atol=2e-15
                    )

                    output = Path(directory) / "mshio-output.msh"
                    mshio.save_msh(str(output), spec)
                    gmsh.clear()
                    gmsh.open(str(output))
                    native_types, native_tags, native_nodes = gmsh.model.mesh.getElements(
                        dim, 1
                    )
                    np.testing.assert_array_equal(native_types, [element_type])
                    np.testing.assert_array_equal(native_tags[0], [1, 2])
                    np.testing.assert_array_equal(native_nodes[0], connectivity)
                    loaded_tags, loaded_xyz, _ = gmsh.model.mesh.getNodes()
                    loaded_nodes = {
                        int(tag): tuple(point)
                        for tag, point in zip(loaded_tags, loaded_xyz.reshape(-1, 3))
                    }
                    self.assertEqual(set(loaded_nodes), set(map(int, tags)))
                    np.testing.assert_allclose(
                        [loaded_nodes[int(tag)] for tag in tags], xyz,
                        rtol=2e-15, atol=2e-15
                    )

    def test_native_gmsh_new_family_records_in_all_encodings(self):
        with tempfile.TemporaryDirectory() as directory:
            for element_type, count, dim in NATIVE_TYPES:
                self.assert_native_roundtrip(directory, element_type, count, dim,
                                             ("2.2", "4.1"))

    def test_native_gmsh_trihedron_in_msh41(self):
        # Gmsh's legacy writer explicitly subtracts trihedra from its element
        # count and does not emit them; MshIO still tests their fixed-width
        # representation in all four encodings in tests/test_elements.cpp.
        with tempfile.TemporaryDirectory() as directory:
            self.assert_native_roundtrip(directory, 140, 4, 3, ("4.1",))

    def test_properties_agree_for_representative_new_families(self):
        # The API builds a nodal basis; do not apply it to XFEM/border elements
        # whose fixed MSH counts exist but whose nodal bases are not implemented.
        for element_type in (36, 42, 62, 71, 92, 118, 137):
            with self.subTest(element_type=element_type):
                _, dim, _, count, _, _ = gmsh.model.mesh.getElementProperties(
                    element_type
                )
                self.assertEqual(mshio.nodes_per_element(element_type), count)
                self.assertEqual(mshio.get_element_dim(element_type), dim)
