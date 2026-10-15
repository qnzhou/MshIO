"""Independent binary wire fixtures, with optional released-Gmsh oracles."""

from pathlib import Path
import struct
import sys
import tempfile
import unittest

import mshio
import numpy as np

try:
    import gmsh
except ModuleNotFoundError as error:
    if error.name != "gmsh":
        raise
    gmsh = None


NATIVE = "<" if sys.byteorder == "little" else ">"
OPPOSITE = ">" if NATIVE == "<" else "<"
COORDINATES = ((.125, -1.75, 2.5), (3.625, .5, -.875))
AFFINE = (1., 0., 0., 3.5, 0., 1., 0., 2.25,
          0., 0., 1., -3.375, 0., 0., 0., 1.)
DATA = (("NodeData", "node samples", 17, (-3.125,)),
        ("ElementData", "element samples", 93, (12.375,)),
        ("ElementNodeData", "element node samples", 93, (.625, -7.75)))
SCHEME = (b'$InterpolationScheme\n"basis with \\literal\\ paths"\n'
          b'1\n2\n2\n2 2\n1.25 -0.5\n0.125 2.75\n'
          b'2 1\n0\n1\n$EndInterpolationScheme\n')
CURVE_DATA = (.125, -1.75, 2.5, -.625, 3.625, .5, -.875, 1.875)
SURFACE_DATA = tuple((index - 13) / 8 for index in range(33))


def mesh_header(version, order):
    return (f"$MeshFormat\n{version} 1 8\n".encode("ascii")
            + struct.pack(order + "i", 1) + b"\n$EndMeshFormat\n")


def binary_section(name, payload):
    return f"${name}\n".encode("ascii") + payload + f"\n$End{name}\n".encode("ascii")


def mesh_fixture(version, order):
    """Pack wire fields directly: never derive input bytes from MshIO output."""
    def pack(fmt, *values):
        return struct.pack(order + fmt, *values)

    second = 2**32 + 29 if version == "4.1" else 29
    raw = mesh_header(version, order)
    raw += b'$PhysicalNames\n1\n1 7 "oriented edge \\literal\\ path"\n$EndPhysicalNames\n'
    if version == "4.1":
        entities = pack("4Q", 2, 1, 0, 0)
        for tag, xyz in enumerate(COORDINATES, 1):
            entities += pack("i3dQ", tag, *xyz, 0)
        entities += pack("i6dQiQ2i", 11, .125, -1.75, -.875,
                         3.625, .5, 2.5, 1, 7, 2, -1, 2)
        raw += binary_section("Entities", entities)
        nodes = pack("4Q", 2, 2, 17, second)
        for entity, tag, xyz in zip((1, 2), (17, second), COORDINATES):
            nodes += pack("3iQQ3d", 0, entity, 0, 1, tag, *xyz)
        raw += binary_section("Nodes", nodes)
        elements = pack("4Q", 3, 3, 91, 93)
        elements += pack("3iQ2Q", 0, 1, 15, 1, 91, 17)
        elements += pack("3iQ2Q", 0, 2, 15, 1, 92, second)
        elements += pack("3iQ3Q", 1, 11, 1, 1, 93, 17, second)
        raw += binary_section("Elements", elements)
        periodic = pack("Q3iQ", 1, 0, 2, 1, 16)
        periodic += pack("16dQ2Q", *AFFINE, 1, second, 17)
        raw += binary_section("Periodic", periodic)
    else:
        raw += b"$Nodes\n2\n"
        for tag, xyz in zip((17, second), COORDINATES):
            raw += pack("i3d", tag, *xyz)
        raw += b"\n$EndNodes\n$Elements\n3\n"
        # Two point elements establish the entities used by the periodic link.
        raw += pack("3i4i4i", 15, 2, 2, 91, 0, 1, 17, 92, 0, 2, second)
        raw += pack("3i5i", 1, 1, 2, 93, 7, 11, 17, second)
        raw += b"\n$EndElements\n$Periodic\n1\n0 2 1\nAffine "
        raw += " ".join(map(str, AFFINE)).encode("ascii")
        raw += f"\n1\n{second} 17\n$EndPeriodic\n".encode("ascii")
    # Data tags stay int32 even in MSH 4.1; headers stay text in binary files.
    for section, name, tag, values in DATA:
        raw += f'${section}\n1\n"{name}"\n1\n0.375\n3\n0\n1\n1\n'.encode("ascii")
        raw += pack("i", tag)
        if section == "ElementNodeData":
            raw += pack("i", 2)
        raw += pack(f"{len(values)}d", *values)
        raw += f"\n$End{section}\n".encode("ascii")
    return raw


def optional_fixture(order):
    """Exercise metadata independently of Gmsh's model-topology constraints."""
    def pack(fmt, *values):
        return struct.pack(order + fmt, *values)

    raw = mesh_header("4.1", order)
    # One partitioned entity of every dimension, with signed boundary tags.
    partitioned = pack("QQ2i4Q", 3, 1, 39, 2, 1, 1, 1, 1)
    for dimension in range(4):
        partitioned += pack("3iQ2i", 31 + dimension, dimension, 11 + dimension, 2, 1, 3)
        if dimension == 0:
            partitioned += pack("3d", .125, -.625, 1.875)
        else:
            partitioned += pack("6d", -.125, -.625, -1.875, 2.25, 3.5, 4.75)
        partitioned += pack("Qi", 1, 71 + dimension)
        if dimension:
            partitioned += pack("Q2i", 2, 30 + dimension, -(40 + dimension))
    raw += binary_section("PartitionedEntities", partitioned)
    raw += binary_section("GhostElements", pack("QQiQ2i", 1, 2**32 + 93, 1, 2, 2, 3))
    params = pack("2QiQ8d", 1, 1, 11, 2, *CURVE_DATA)
    params += pack("iQQ33d3i", 12, 3, 1, *SURFACE_DATA, 0, 2, 1)
    raw += binary_section("Parametrizations", params)
    # This entire section is text, even following opposite-endian binary data.
    return raw + SCHEME


class EndianAssertions:
    def assert_native_output(self, path, version):
        raw = path.read_bytes()
        prefix = f"$MeshFormat\n{version} 1 8\n".encode("ascii")
        self.assertTrue(raw.startswith(prefix))
        marker = raw[len(prefix):len(prefix) + 4]
        self.assertEqual(marker, struct.pack(NATIVE + "i", 1))
        self.assertNotEqual(marker, struct.pack(OPPOSITE + "i", 1))
        self.assertTrue(raw[len(prefix) + 4:].startswith(b"\n$EndMeshFormat\n"))
        return raw

    def assert_mesh(self, spec, version):
        second = 2**32 + 29 if version == "4.1" else 29
        self.assertEqual(spec.mesh_format.version, version)
        self.assertEqual(spec.mesh_format.file_type, 1)
        self.assertEqual(spec.mesh_format.data_size, 8)
        self.assertEqual(spec.nodes.num_nodes, 2)
        self.assertEqual((spec.nodes.min_node_tag, spec.nodes.max_node_tag), (17, second))
        nodes = {}
        for block in spec.nodes.entity_blocks:
            self.assertEqual(block.parametric, 0)
            for index, tag in enumerate(block.tags):
                nodes[tag] = tuple(block.data[3 * index:3 * index + 3])
        self.assertEqual(nodes, dict(zip((17, second), COORDINATES)))
        self.assertEqual(spec.elements.num_elements, 3)
        self.assertEqual((spec.elements.min_element_tag, spec.elements.max_element_tag), (91, 93))
        self.assertEqual([(block.entity_dim, block.entity_tag, block.element_type, block.data.tolist())
                          for block in spec.elements.entity_blocks],
                         [(0, 1, 15, [91, 17]), (0, 2, 15, [92, second]),
                          (1, 11, 1, [93, 17, second])])
        self.assertEqual([(group.dim, group.tag, group.name) for group in spec.physical_groups],
                         [(1, 7, r"oriented edge \literal\ path")])
        self.assertEqual(spec.entities.curves[0].physical_group_tags, [7])
        if version == "4.1":
            self.assertGreater(second, 2**32)
            self.assertEqual(spec.entities.curves[0].boundary_point_tags, [-1, 2])
            self.assertEqual((spec.entities.curves[0].min_z, spec.entities.curves[0].max_x),
                             (-.875, 3.625))
        else:
            self.assertEqual(spec.elements.entity_blocks[2].legacy_tags, [7, 11])
        self.assertEqual(len(spec.periodic_links), 1)
        link = spec.periodic_links[0]
        self.assertEqual((link.entity_dim, link.slave_tag, link.master_tag), (0, 2, 1))
        self.assertEqual(link.affine_transform, list(AFFINE))
        self.assertEqual([(pair.slave_tag, pair.master_tag) for pair in link.node_pairs],
                         [(second, 17)])
        for field, (_, name, tag, values) in zip(
                ("node_data", "element_data", "element_node_data"), DATA):
            collection = getattr(spec, field)
            self.assertEqual(len(collection), 1)
            data = collection[0]
            self.assertEqual(data.header.string_tags, [name])
            self.assertEqual(data.header.real_tags, [.375])
            self.assertEqual(data.header.int_tags, [0, 1, 1])
            self.assertEqual(len(data.entries), 1)
            self.assertEqual(data.entries[0].tag, tag)
            np.testing.assert_array_equal(data.entries[0].data, values)
        self.assertEqual(spec.element_node_data[0].entries[0].num_nodes_per_elements, 2)

    def assert_optional(self, spec):
        entities = spec.partitioned_entities
        self.assertEqual(entities.num_partitions, 3)
        self.assertEqual([(ghost.tag, ghost.partition) for ghost in entities.ghost_entities],
                         [(39, 2)])
        for dimension, collection in enumerate(
                (entities.points, entities.curves, entities.surfaces, entities.volumes)):
            self.assertEqual(len(collection), 1)
            entity = collection[0]
            self.assertEqual((entity.tag, entity.parent_dim, entity.parent_tag),
                             (31 + dimension, dimension, 11 + dimension))
            self.assertEqual(entity.partitions, [1, 3])
            self.assertEqual(entity.physical_group_tags, [71 + dimension])
            if dimension == 0:
                self.assertEqual((entity.x, entity.y, entity.z), (.125, -.625, 1.875))
            else:
                self.assertEqual(tuple(getattr(entity, name) for name in
                                       ("min_x", "min_y", "min_z", "max_x", "max_y", "max_z")),
                                 (-.125, -.625, -1.875, 2.25, 3.5, 4.75))
                self.assertEqual(getattr(entity, ("", "boundary_point_tags", "boundary_curve_tags",
                                                 "boundary_surface_tags")[dimension]),
                                 [30 + dimension, -(40 + dimension)])
        self.assertEqual([(ghost.element_tag, ghost.partition, ghost.ghost_partitions)
                          for ghost in spec.ghost_elements], [(2**32 + 93, 1, [2, 3])])
        self.assertEqual(len(spec.parametrizations.curves), 1)
        curve = spec.parametrizations.curves[0]
        self.assertEqual((curve.tag, curve.num_nodes, curve.data), (11, 2, list(CURVE_DATA)))
        self.assertEqual(len(spec.parametrizations.surfaces), 1)
        surface = spec.parametrizations.surfaces[0]
        self.assertEqual((surface.tag, surface.num_nodes, surface.num_triangles), (12, 3, 1))
        self.assertEqual(surface.data, list(SURFACE_DATA))
        self.assertEqual(surface.triangle_indices, [0, 2, 1])
        self.assertEqual(len(spec.interpolation_schemes), 1)
        scheme = spec.interpolation_schemes[0]
        self.assertEqual(scheme.name, r"basis with \literal\ paths")
        self.assertEqual(len(scheme.topologies), 1)
        topology = scheme.topologies[0]
        self.assertEqual(topology.element_topology, 2)
        self.assertEqual([(matrix.num_rows, matrix.num_columns, matrix.data)
                          for matrix in topology.matrices],
                         [(2, 2, [1.25, -.5, .125, 2.75]), (2, 1, [0., 1.])])


class BinaryEndiannessTests(EndianAssertions, unittest.TestCase):
    def test_independent_mesh_fixtures_decode_and_write_native(self):
        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for order in (NATIVE, OPPOSITE):
                    with self.subTest(version=version, order=order):
                        source = Path(directory) / "source.msh"
                        target = Path(directory) / "native.msh"
                        source.write_bytes(mesh_fixture(version, order))
                        spec = mshio.load_msh(str(source))
                        self.assert_mesh(spec, version)
                        mshio.save_msh(str(target), spec)
                        raw = self.assert_native_output(target, version)
                        self.assertIn(b'"oriented edge \\literal\\ path"', raw)
                        for _, name, _, _ in DATA:
                            self.assertIn(f'"{name}"\n'.encode("ascii"), raw)
                        if version == "2.2":
                            self.assertIn(b"\nAffine ", raw)
                        self.assert_mesh(mshio.load_msh(str(target)), version)

    def test_optional_binary_sections_and_ascii_interpolation(self):
        with tempfile.TemporaryDirectory() as directory:
            for order in (NATIVE, OPPOSITE):
                with self.subTest(order=order):
                    source = Path(directory) / "optional.msh"
                    target = Path(directory) / "native.msh"
                    source.write_bytes(optional_fixture(order))
                    spec = mshio.load_msh(str(source))
                    self.assert_optional(spec)
                    mshio.save_msh(str(target), spec)
                    raw = self.assert_native_output(target, "4.1")
                    text = raw.split(b"$InterpolationScheme\n", 1)[1]
                    text = text.split(b"$EndInterpolationScheme", 1)[0]
                    self.assertIn(b'"basis with \\literal\\ paths"', text)
                    text.decode("ascii")
                    self.assert_optional(mshio.load_msh(str(target)))


@unittest.skipIf(gmsh is None, "Gmsh endian oracle requires pip install '.[test]'")
class GmshEndiannessTests(EndianAssertions, unittest.TestCase):
    def setUp(self):
        gmsh.initialize([], readConfigFiles=False)
        self.addCleanup(gmsh.finalize)
        gmsh.option.setNumber("General.Terminal", 0)
        # The 2.2 reader otherwise deliberately discards the valid text section.
        gmsh.option.setNumber("Mesh.IgnorePeriodicity", 0)

    def assert_gmsh_mesh(self, version):
        second = 2**32 + 29 if version == "4.1" else 29
        tags, coordinates, _ = gmsh.model.mesh.getNodes()
        actual = {int(tag): tuple(xyz) for tag, xyz in
                  zip(tags, np.asarray(coordinates).reshape(-1, 3))}
        self.assertEqual(actual, dict(zip((17, second), COORDINATES)))
        for tag, kind, nodes in ((91, 15, [17]), (92, 15, [second]),
                                 (93, 1, [17, second])):
            element_type, connectivity, dimension, entity = gmsh.model.mesh.getElement(tag)
            self.assertEqual(element_type, kind)
            np.testing.assert_array_equal(connectivity, nodes)
            self.assertEqual((dimension, entity), (0, tag - 90) if tag < 93 else (1, 11))
        self.assertEqual(gmsh.model.getPhysicalGroups(), [(1, 7)])
        self.assertEqual(gmsh.model.getPhysicalName(1, 7), r"oriented edge \literal\ path")
        np.testing.assert_array_equal(gmsh.model.getEntitiesForPhysicalGroup(1, 7), [11])
        if version == "4.1":
            self.assertEqual({abs(tag) for _, tag in gmsh.model.getBoundary([(1, 11)])}, {1, 2})
        master, slaves, masters, affine = gmsh.model.mesh.getPeriodicNodes(0, 2)
        self.assertEqual(master, 1)
        self.assertEqual(dict(zip(map(int, slaves), map(int, masters))), {second: 17})
        np.testing.assert_array_equal(affine, AFFINE)
        views = gmsh.view.getTags()
        self.assertEqual(len(views), 3)
        actual_data = {}
        for view in views:
            kind, tags, values, time, components = gmsh.view.getModelData(int(view), 0)
            actual_data[kind] = (list(map(int, tags)), [list(value) for value in values], time, components)
            name = gmsh.option.getString(f"View[{gmsh.view.getIndex(int(view))}].Name")
            self.assertEqual(name, next(name for section, name, _, _ in DATA if section == kind))
        self.assertEqual(actual_data,
                         {section: ([tag], [list(values)], .375, 1)
                          for section, _, tag, values in DATA})

    def test_gmsh_loads_both_source_orders_and_native_mshio_output(self):
        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for order in (NATIVE, OPPOSITE):
                    with self.subTest(version=version, order=order):
                        source = Path(directory) / "independent.msh"
                        target = Path(directory) / "mshio.msh"
                        source.write_bytes(mesh_fixture(version, order))
                        # Legacy attributes can survive reloads: never reuse a model.
                        gmsh.clear()
                        gmsh.open(str(source))
                        self.assert_gmsh_mesh(version)
                        spec = mshio.load_msh(str(source))
                        self.assert_mesh(spec, version)
                        mshio.save_msh(str(target), spec)
                        self.assert_native_output(target, version)
                        gmsh.clear()
                        gmsh.open(str(target))
                        self.assert_gmsh_mesh(version)


if __name__ == "__main__":
    unittest.main()
