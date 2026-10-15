"""Independent loader checks; install with pip install '.[test]' to enable."""

import math
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


DATA_DIR = Path(__file__).resolve().parent.parent / "data"


@unittest.skipIf(gmsh is None, "Gmsh loader checks require pip install '.[test]'")
class GmshLoaderTests(unittest.TestCase):
    def setUp(self):
        # Do not read user configuration or pass test-runner arguments to Gmsh.
        gmsh.initialize([], readConfigFiles=False)
        self.addCleanup(gmsh.finalize)
        gmsh.option.setNumber("General.Terminal", 0)

    def test_mesh_fields_and_physical_membership_in_all_encodings(self):
        spec = mshio.load_msh(str(DATA_DIR / "test_2.2_ascii.msh"))
        group = mshio.PhysicalGroup()
        group.dim = 2
        group.tag = 99
        group.name = "material"
        spec.physical_groups = [group]

        expected_nodes = {
            1: (0.0, 0.0, 0.0),
            2: (1.0, 0.0, 0.0),
            3: (1.0, 1.0, 0.0),
            4: (0.0, 1.0, 0.0),
            5: (2.0, 0.0, 0.0),
            6: (2.0, 1.0, 0.0),
        }
        expected_elements = {1: (1, 2, 3, 4), 2: (2, 5, 6, 3)}
        expected_values = {
            1: (0.0,),
            2: (0.1,),
            3: (0.2,),
            4: (0.0,),
            5: (0.2,),
            6: (0.4,),
        }
        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for encoding in (0, 1):
                    with self.subTest(version=version, encoding=encoding):
                        spec.mesh_format.version = version
                        spec.mesh_format.file_type = encoding
                        path = Path(directory) / "output.msh"
                        mshio.save_msh(str(path), spec)
                        gmsh.clear()
                        gmsh.open(str(path))

                        tags, coordinates, _ = gmsh.model.mesh.getNodes()
                        actual_nodes = {
                            int(tag): tuple(xyz)
                            for tag, xyz in zip(tags, coordinates.reshape(-1, 3))
                        }
                        self.assertEqual(actual_nodes, expected_nodes)
                        # Check entity classification, not only global connectivity:
                        # the former MSH 2.2 bug wrote entity 2 as physical group 2.
                        types, element_tags, connectivity = gmsh.model.mesh.getElements(
                            2, 2
                        )
                        np.testing.assert_array_equal(types, [3])
                        actual_elements = {
                            int(tag): tuple(int(node) for node in nodes)
                            for tag, nodes in zip(
                                element_tags[0], connectivity[0].reshape(-1, 4)
                            )
                        }
                        self.assertEqual(actual_elements, expected_elements)
                        self.assertEqual(gmsh.model.getPhysicalGroups(), [(2, 99)])
                        self.assertEqual(gmsh.model.getPhysicalName(2, 99), "material")
                        np.testing.assert_array_equal(
                            gmsh.model.getEntitiesForPhysicalGroup(2, 99), [2]
                        )

                        views = gmsh.view.getTags()
                        self.assertEqual(len(views), 1)
                        kind, tags, values, time, components = gmsh.view.getModelData(
                            int(views[0]), 0
                        )
                        self.assertEqual(kind, "NodeData")
                        self.assertEqual(components, 1)
                        self.assertEqual(time, 0.0)
                        self.assertEqual(
                            {
                                int(tag): tuple(value)
                                for tag, value in zip(tags, values)
                            },
                            expected_values,
                        )

    def test_entity_membership_is_used_when_legacy_tags_are_absent(self):
        spec = mshio.load_msh(str(DATA_DIR / "test_2.2_ascii.msh"))
        # Exercise export from the 4.1-shaped model, rather than replaying the
        # input file's legacy metadata. No physical name is needed for membership.
        for index, block in enumerate(spec.elements.entity_blocks):
            block.legacy_tags = []
            spec.elements.entity_blocks[index] = block
        spec.mesh_format.version = "2.2"
        with tempfile.TemporaryDirectory() as directory:
            for encoding in (0, 1):
                with self.subTest(encoding=encoding):
                    spec.mesh_format.file_type = encoding
                    path = Path(directory) / "derived.msh"
                    mshio.save_msh(str(path), spec)
                    gmsh.clear()
                    gmsh.open(str(path))
                    np.testing.assert_array_equal(
                        gmsh.model.getPhysicalGroupsForEntity(2, 2), [99]
                    )
                    np.testing.assert_array_equal(
                        gmsh.model.getEntitiesForPhysicalGroup(2, 99), [2]
                    )

    def test_floating_point_output_matches_gmsh_writer(self):
        values = [1.2345678901234567, math.pi, math.nextafter(1.0, 2.0),
                  0.1, 1.2345678901234567e-120, 1.2345678901234567e120]
        time = 0.12345678901234567
        coordinates = np.zeros((6, 3))
        coordinates[:, 0] = values
        datasets = [
            ("NodeData", list(range(1, 7)), [[value] for value in values], 1),
            ("ElementData", [1, 2], [values[:3], values[3:]], 3),
            ("ElementNodeData", [1, 2], [values[:4], values[2:]], 1),
        ]

        # Produce independent expected files with Gmsh's own writer, not a
        # Python reimplementation of its floating-point formatting.
        gmsh.model.add("precision")
        gmsh.model.addDiscreteEntity(2, 2)
        gmsh.model.mesh.addNodes(2, 2, list(range(1, 7)), coordinates.ravel())
        gmsh.model.mesh.addElementsByType(2, 3, [1, 2], [1, 2, 3, 4, 2, 5, 6, 3])
        views = []
        for kind, tags, entries, components in datasets:
            view = gmsh.view.add(kind)
            gmsh.view.addModelData(view, 0, "precision", kind, tags, entries, time, components)
            gmsh.view.setInterpolationMatrices(view, "Quadrangle", -1, [], [])
            views.append(view)
        gmsh.option.setNumber("Mesh.SaveAll", 1)
        gmsh.option.setNumber("PostProcessing.SaveMesh", 0)

        spec = mshio.load_msh(str(DATA_DIR / "test_2.2_ascii.msh"))
        block = spec.nodes.entity_blocks[0]
        block.data = coordinates.ravel()
        spec.nodes.entity_blocks[0] = block
        spec.node_data.clear()
        for kind, tags, entries, components in datasets:
            data = mshio.Data()
            data.header.string_tags = [kind]
            data.header.real_tags = [time]
            data.header.int_tags = [0, components, len(tags)]
            for tag, entry_values in zip(tags, entries):
                entry = mshio.DataEntry()
                entry.tag = tag
                entry.data = entry_values
                if kind == "ElementNodeData":
                    entry.num_nodes_per_elements = 4
                data.entries.append(entry)
            getattr(spec, {"NodeData": "node_data", "ElementData": "element_data",
                           "ElementNodeData": "element_node_data"}[kind]).append(data)

        def snapshot(path):
            gmsh.clear()
            gmsh.open(str(path))
            tags, xyz, _ = gmsh.model.mesh.getNodes()
            nodes = sorted((int(tag), tuple(point))
                           for tag, point in zip(tags, xyz.reshape(-1, 3)))
            data = {}
            for view in gmsh.view.getTags():
                kind, tags, entries, timestamp, components = gmsh.view.getModelData(int(view), 0)
                data[kind] = (timestamp, components,
                              sorted((int(tag), tuple(entry))
                                     for tag, entry in zip(tags, entries)))
            return nodes, data

        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for encoding in (0, 1):
                    gmsh.option.setNumber("Mesh.MshFileVersion", float(version))
                    gmsh.option.setNumber("Mesh.Binary", encoding)
                    reference = Path(directory) / f"reference-{version}-{encoding}.msh"
                    gmsh.write(str(reference))
                    for view in views:
                        gmsh.view.write(view, str(reference), True)

            for version in ("2.2", "4.1"):
                for encoding in (0, 1):
                    with self.subTest(version=version, encoding=encoding):
                        reference = Path(directory) / f"reference-{version}-{encoding}.msh"
                        expected = snapshot(reference)
                        spec.mesh_format.version = version
                        spec.mesh_format.file_type = encoding
                        actual = Path(directory) / "mshio.msh"
                        mshio.save_msh(str(actual), spec)
                        self.assertEqual(snapshot(actual), expected)


if __name__ == "__main__":
    unittest.main()
