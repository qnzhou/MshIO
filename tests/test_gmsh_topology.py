"""Topology binding and released-Gmsh wire interoperability checks."""

from pathlib import Path
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


def topology_snapshot(spec):
    entities = spec.partitioned_entities
    records = []
    for dimension, collection in enumerate(
        (entities.points, entities.curves, entities.surfaces, entities.volumes)
    ):
        for entity in collection:
            coordinates = (
                (entity.x, entity.y, entity.z)
                if dimension == 0
                else tuple(getattr(entity, field) for field in
                           ("min_x", "min_y", "min_z", "max_x", "max_y", "max_z"))
            )
            boundary = (() if dimension == 0 else tuple(getattr(
                entity, ("", "boundary_point_tags", "boundary_curve_tags",
                         "boundary_surface_tags")[dimension]
            )))
            records.append((dimension, entity.tag, entity.parent_dim, entity.parent_tag,
                            tuple(entity.partitions), coordinates,
                            tuple(entity.physical_group_tags), boundary))
    return (
        entities.num_partitions,
        tuple((entry.tag, entry.partition) for entry in entities.ghost_entities),
        tuple(records),
        tuple((link.entity_dim, link.slave_tag, link.master_tag,
               tuple(link.affine_transform),
               tuple((pair.slave_tag, pair.master_tag) for pair in link.node_pairs))
              for link in spec.periodic_links),
        tuple((entry.element_tag, entry.partition, tuple(entry.ghost_partitions))
              for entry in spec.ghost_elements),
    )


class TopologyBindingTests(unittest.TestCase):
    def test_metadata_object_vectors_assign_back_and_roundtrip(self):
        spec = mshio.MshSpec()
        entities = spec.partitioned_entities
        entities.num_partitions = 3
        for dimension, name in enumerate(("Point", "Curve", "Surface", "Volume")):
            entity = getattr(mshio, "Partitioned" + name + "Entity")()
            entity.tag = 10 + dimension
            entity.parent_dim = dimension
            entity.parent_tag = 1 + dimension
            entity.partitions = [1, 2]
            entity.physical_group_tags = [20 + dimension]
            collection = getattr(entities, ("points", "curves", "surfaces", "volumes")[dimension])
            collection.append(entity)
            stored = collection[0]
            stored.parent_tag = 7 + dimension
            self.assertEqual(collection[0].parent_tag, 1 + dimension)
            collection[0] = stored
            self.assertEqual(collection[0].parent_tag, 7 + dimension)
        for collection, field, tags in (
            (entities.curves, "boundary_point_tags", [10, -11]),
            (entities.surfaces, "boundary_curve_tags", [11, -12]),
            (entities.volumes, "boundary_surface_tags", [12, -13]),
        ):
            stored = collection[0]
            setattr(stored, field, tags)
            collection[0] = stored
        ghost_entity = mshio.GhostEntity()
        ghost_entity.tag = 30
        ghost_entity.partition = 2
        entities.ghost_entities = [ghost_entity]
        ghost_entity = entities.ghost_entities[0]
        ghost_entity.partition = 3
        entities.ghost_entities[0] = ghost_entity
        self.assertEqual(entities.ghost_entities[0].partition, 3)

        link = mshio.PeriodicLink()
        link.entity_dim = 1
        link.slave_tag = 11
        link.master_tag = 14
        pair = mshio.PeriodicNodePair()
        pair.slave_tag = 2**32 + 10
        pair.master_tag = 2**32 + 11
        link.node_pairs = [pair]
        spec.periodic_links = [link]
        stored_link = spec.periodic_links[0]
        stored_pair = stored_link.node_pairs[0]
        stored_pair.master_tag = 2**32 + 12
        stored_link.node_pairs[0] = stored_pair
        spec.periodic_links[0] = stored_link
        self.assertEqual(spec.periodic_links[0].node_pairs[0].master_tag, 2**32 + 12)
        # Numeric metadata vectors intentionally retain the existing list-copy convention.
        copied_partitions = entities.points[0].partitions
        copied_partitions.append(3)
        self.assertEqual(entities.points[0].partitions, [1, 2])
        ghost = mshio.GhostElement()
        ghost.element_tag = 2**32 + 10
        ghost.partition = 1
        ghost.ghost_partitions = [2, 3]
        spec.ghost_elements = [ghost]
        stored_ghost = spec.ghost_elements[0]
        stored_ghost.partition = 2
        spec.ghost_elements[0] = stored_ghost
        expected = topology_snapshot(spec)
        with tempfile.TemporaryDirectory() as directory:
            for encoding in (0, 1):
                with self.subTest(encoding=encoding):
                    spec.mesh_format.file_type = encoding
                    path = Path(directory) / "metadata.msh"
                    mshio.save_msh(str(path), spec)
                    actual = mshio.load_msh(str(path))
                    self.assertEqual(topology_snapshot(actual), expected)
            spec.mesh_format.version = "2.2"
            with self.assertRaises(Exception):
                mshio.save_msh(str(Path(directory) / "lossy.msh"), spec)

    def test_legacy_periodic_affine_and_no_affine(self):
        spec = mshio.MshSpec()
        link = mshio.PeriodicLink()
        link.entity_dim, link.slave_tag, link.master_tag = 1, 2, 4
        link.affine_transform = [1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        pair = mshio.PeriodicNodePair()
        pair.slave_tag, pair.master_tag = 2, 4
        link.node_pairs.append(pair)
        spec.periodic_links.append(link)
        no_affine = mshio.PeriodicLink()
        no_affine.entity_dim, no_affine.slave_tag, no_affine.master_tag = 0, 2, 1
        spec.periodic_links.append(no_affine)
        expected = topology_snapshot(spec)
        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for encoding in (0, 1):
                    with self.subTest(version=version, encoding=encoding):
                        spec.mesh_format.version = version
                        spec.mesh_format.file_type = encoding
                        path = Path(directory) / "periodic.msh"
                        mshio.save_msh(str(path), spec)
                        self.assertEqual(topology_snapshot(mshio.load_msh(str(path))), expected)
                        if version == "2.2":
                            self.assertIn(b"Affine ", path.read_bytes())


@unittest.skipIf(gmsh is None, "Gmsh topology oracle requires pip install '.[test]'")
class GmshTopologyTests(unittest.TestCase):
    def setUp(self):
        gmsh.initialize([], readConfigFiles=False)
        self.addCleanup(gmsh.finalize)
        gmsh.option.setNumber("General.Terminal", 0)
        gmsh.option.setNumber("Mesh.SaveAll", 1)

    def periodic_square(self):
        gmsh.model.add("periodic-square")
        points = [gmsh.model.geo.addPoint(x, y, 0, 0.25)
                  for x, y in ((0, 0), (1, 0), (1, 1), (0, 1))]
        curves = [gmsh.model.geo.addLine(points[i], points[(i + 1) % 4])
                  for i in range(4)]
        wire = gmsh.model.geo.addCurveLoop(curves)
        gmsh.model.geo.addPlaneSurface([wire])
        gmsh.model.geo.synchronize()
        gmsh.model.mesh.setPeriodic(1, [2], [4],
                                   [1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1])
        gmsh.model.mesh.generate(2)

    def partition_snapshot(self):
        records = []
        for dim, tag in gmsh.model.getEntities():
            partitions = tuple(int(value) for value in gmsh.model.getPartitions(dim, tag))
            elements, owners = gmsh.model.mesh.getGhostElements(dim, tag)
            ghosts = tuple(sorted((int(element), int(owner))
                                  for element, owner in zip(elements, owners)))
            if partitions or ghosts:
                records.append((dim, tag, gmsh.model.getParent(dim, tag), partitions, ghosts))
        return records

    def assert_periodic(self, expected):
        master, slaves, masters, affine = gmsh.model.mesh.getPeriodicNodes(1, 2)
        self.assertEqual(master, expected[0])
        self.assertEqual(dict(zip(map(int, slaves), map(int, masters))), expected[1])
        np.testing.assert_array_equal(affine, expected[2])

    def test_periodic_is_compatible_with_gmsh_in_all_encodings(self):
        # Gmsh deliberately ignores MSH 2.2 periodic sections by default.
        gmsh.option.setNumber("Mesh.IgnorePeriodicity", 0)
        with tempfile.TemporaryDirectory() as directory:
            for version in ("2.2", "4.1"):
                for encoding in (0, 1):
                    with self.subTest(version=version, encoding=encoding):
                        # Reloaded legacy sections can remain as Gmsh model
                        # attributes; start fresh before changing file versions.
                        gmsh.clear()
                        self.periodic_square()
                        master, slaves, masters, affine = gmsh.model.mesh.getPeriodicNodes(1, 2)
                        expected = master, dict(zip(map(int, slaves), map(int, masters))), affine.copy()
                        self.assertEqual(master, 4)
                        self.assertGreater(len(slaves), 0)
                        gmsh.option.setNumber("Mesh.MshFileVersion", float(version))
                        gmsh.option.setNumber("Mesh.Binary", encoding)
                        source = Path(directory) / "gmsh.msh"
                        gmsh.write(str(source))
                        spec = mshio.load_msh(str(source))
                        link = next(link for link in spec.periodic_links
                                    if link.entity_dim == 1 and link.slave_tag == 2)
                        self.assertEqual(link.master_tag, 4)
                        self.assertEqual({pair.slave_tag: pair.master_tag for pair in link.node_pairs},
                                         expected[1])
                        np.testing.assert_array_equal(link.affine_transform, expected[2])
                        output = Path(directory) / "mshio.msh"
                        mshio.save_msh(str(output), spec)
                        gmsh.clear()
                        gmsh.open(str(output))
                        self.assert_periodic(expected)

    def test_partitioned_all_dimensions_and_ghost_elements_use_gmsh_wire(self):
        gmsh.model.add("partitioned-box")
        gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
        gmsh.model.occ.synchronize()
        gmsh.option.setNumber("Mesh.MeshSizeMin", 0.4)
        gmsh.option.setNumber("Mesh.MeshSizeMax", 0.4)
        gmsh.model.mesh.generate(3)
        gmsh.option.setNumber("Mesh.PartitionCreateTopology", 1)
        gmsh.option.setNumber("Mesh.PartitionCreateGhostCells", 1)
        gmsh.model.mesh.partition(2)
        gmsh.option.setNumber("Mesh.MshFileVersion", 4.1)
        with tempfile.TemporaryDirectory() as directory:
            for encoding in (0, 1):
                with self.subTest(encoding=encoding):
                    gmsh.option.setNumber("Mesh.Binary", encoding)
                    source = Path(directory) / "gmsh-partitioned.msh"
                    gmsh.write(str(source))
                    spec = mshio.load_msh(str(source))
                    partitions = spec.partitioned_entities
                    self.assertEqual(partitions.num_partitions, 2)
                    for collection in (partitions.points, partitions.curves,
                                       partitions.surfaces, partitions.volumes):
                        self.assertGreater(len(collection), 0)
                    self.assertGreater(len(partitions.ghost_entities), 0)
                    self.assertGreater(len(spec.ghost_elements), 0)
                    self.assertTrue(all(entry.ghost_partitions for entry in spec.ghost_elements))
                    expected_metadata = topology_snapshot(spec)
                    # Compare semantics after Gmsh loads its own file and MshIO's file.
                    gmsh.clear()
                    gmsh.open(str(source))
                    expected = self.partition_snapshot()
                    self.assertTrue(expected)
                    output = Path(directory) / "mshio-partitioned.msh"
                    mshio.save_msh(str(output), spec)
                    self.assertEqual(topology_snapshot(mshio.load_msh(str(output))), expected_metadata)
                    gmsh.clear()
                    gmsh.open(str(output))
                    self.assertEqual(self.partition_snapshot(), expected)


if __name__ == "__main__":
    unittest.main()
