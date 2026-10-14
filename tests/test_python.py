import gc
from pathlib import Path
import tempfile
import unittest

import numpy as np
import mshio


DATA_DIR = Path(__file__).resolve().parent.parent / "data"


class BindingOwnershipTests(unittest.TestCase):
    def test_collection_mutation_and_item_copy_lifetime(self):
        spec = mshio.MshSpec()
        block = mshio.NodeBlock()
        block.entity_tag = 1
        block.tags = [1, 2]
        block.data = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0]
        spec.nodes.entity_blocks = [block]
        blocks = spec.nodes.entity_blocks
        copied = blocks[0]
        copied.entity_tag = 2
        self.assertEqual(blocks[0].entity_tag, 1)
        blocks[0] = copied
        self.assertEqual(spec.nodes.entity_blocks[0].entity_tag, 2)

        # Structural edits act on the collection, not on an eagerly copied list.
        blocks.append(block)
        self.assertEqual(spec.nodes.entity_blocks[1].entity_tag, 1)
        del blocks[0]
        self.assertEqual(spec.nodes.entity_blocks[0].entity_tag, 1)
        blocks.extend(blocks)
        self.assertEqual([b.entity_tag for b in blocks], [1, 1])
        spec.nodes.entity_blocks = []
        self.assertEqual(list(blocks), [])
        np.testing.assert_array_equal(copied.tags, [1, 2])
        np.testing.assert_array_equal(copied.data, [0, 1, 2, 3, 4, 5])

        del spec
        gc.collect()
        blocks.append(copied)
        self.assertEqual(blocks[0].entity_tag, 2)

    def test_writable_numeric_snapshots_own_their_storage(self):
        block = mshio.NodeBlock()
        block.tags = [1, 2**63 + 7, 2**64 - 1]
        block.data = [0.25, 1.25, 2.25]
        tags = block.tags
        coordinates = block.data
        self.assertEqual(tags.dtype, np.dtype("uint64"))
        self.assertEqual(coordinates.dtype, np.dtype("float64"))
        tags[0] = 9
        coordinates[0] = 9.25
        self.assertEqual(block.tags[0], 1)
        self.assertEqual(block.data[0], 0.25)

        block.tags = tags
        block.data = coordinates
        tags[0] = 10
        coordinates[0] = 10.25
        self.assertEqual(block.tags[0], 9)
        self.assertEqual(block.data[0], 9.25)
        block.tags = []
        block.data = []
        self.assertEqual(block.tags.shape, (0,))
        self.assertEqual(block.data.shape, (0,))
        del block
        gc.collect()
        np.testing.assert_array_equal(tags, np.array([10, 2**63 + 7, 2**64 - 1], dtype=np.uint64))
        np.testing.assert_array_equal(coordinates, [10.25, 1.25, 2.25])

    def test_array_setters_accept_checked_sequences_and_strides(self):
        block = mshio.NodeBlock()
        source = np.array([0.5, 1.5, 2.5], dtype=np.float64)
        source.flags.writeable = False
        block.data = source
        np.testing.assert_array_equal(block.data, [0.5, 1.5, 2.5])
        block.data = np.arange(6, dtype=np.float64)[::-2]
        np.testing.assert_array_equal(block.data, [5.0, 3.0, 1.0])
        block.data = np.array([0.5, 1.5], dtype=np.float32)
        np.testing.assert_array_equal(block.data, [0.5, 1.5])
        block.data = (3.5, 4.5)
        np.testing.assert_array_equal(block.data, [3.5, 4.5])
        block.tags = np.array([1, 2, 3], dtype=np.int64)
        np.testing.assert_array_equal(block.tags, [1, 2, 3])
        block.tags = np.array([0, 2**63, 2**64 - 1], dtype=np.uint64)
        np.testing.assert_array_equal(block.tags, np.array([0, 2**63, 2**64 - 1], dtype=np.uint64))

    def test_unaligned_and_non_native_arrays_preserve_values(self):
        block = mshio.NodeBlock()
        expected_tags = np.array([2**63 + 7, 2**64 - 1], dtype=np.uint64)
        tags = np.ndarray((2,), dtype=np.uint64, buffer=bytearray(17), offset=1)
        tags[:] = expected_tags
        block.tags = tags
        tags[0] = 1
        np.testing.assert_array_equal(block.tags, expected_tags)

        coordinates = np.ndarray((2,), dtype=np.float64, buffer=bytearray(17), offset=1)
        coordinates[:] = [0.5, 1.5]
        block.data = coordinates
        np.testing.assert_array_equal(block.data, [0.5, 1.5])
        other_endian = ">" if np.little_endian else "<"
        block.tags = expected_tags.astype(other_endian + "u8")
        np.testing.assert_array_equal(block.tags, expected_tags)
        block.data = np.array([0.5, 1.5], dtype=other_endian + "f8")
        np.testing.assert_array_equal(block.data, [0.5, 1.5])

    def test_object_arrays_use_checked_scalar_conversion(self):
        block = mshio.NodeBlock()
        expected_tags = np.array([1, 2**63 + 7, 2**64 - 1], dtype=np.uint64)
        block.tags = np.array([1, 2**63 + 7, 2**64 - 1], dtype=object)
        np.testing.assert_array_equal(block.tags, expected_tags)
        block.data = np.array([0.5, 1.5], dtype=object)
        np.testing.assert_array_equal(block.data, [0.5, 1.5])

    def test_rejected_array_shapes_and_integer_values_preserve_storage(self):
        block = mshio.NodeBlock()
        block.tags = [7, 8]
        invalid_tags = [
            [-1],
            [2**64],
            [1.5],
            np.array([-1], dtype=np.int64),
            np.array([1.5], dtype=np.float64),
            np.array([2**64], dtype=object),
        ]
        for source in invalid_tags:
            with self.subTest(source=source):
                with self.assertRaises((TypeError, RuntimeError)):
                    block.tags = source
                np.testing.assert_array_equal(block.tags, [7, 8])

        block.data = [0.5, 1.5]
        for source in (
            np.array(1.0), np.ones((2, 2)), np.empty((0, 2)),
            np.array(1.0, dtype=object), np.ones((2, 2), dtype=object),
            np.empty((0, 2), dtype=object),
        ):
            with self.subTest(shape=source.shape):
                with self.assertRaises(TypeError):
                    block.data = source
                np.testing.assert_array_equal(block.data, [0.5, 1.5])

    def test_nested_data_edits_and_fixed_width_roundtrip(self):
        spec = mshio.load_msh(str(DATA_DIR / "portability_4.1_ascii.msh"))
        node_block = spec.nodes.entity_blocks[-1]
        expected_tags = np.array(
            [1, 2, 3, 4, 4294967297, 1099511627783, 281474976710665],
            dtype=np.uint64,
        )
        np.testing.assert_array_equal(node_block.tags, expected_tags)
        coordinates = node_block.data
        coordinates[0] = 0.125
        node_block.data = coordinates
        spec.nodes.entity_blocks[-1] = node_block

        data = spec.node_data[0]
        entry = data.entries[0]
        entry.data = np.array([9.0, 8.0, 7.0])
        data.entries[0] = entry
        np.testing.assert_array_equal(spec.node_data[0].entries[0].data, [1, 2, 3])
        spec.node_data[0] = data
        group = spec.physical_groups[0]
        group.name = "edited corner"
        spec.physical_groups[0] = group
        mshio.validate_spec(spec)

        with tempfile.TemporaryDirectory() as directory:
            for encoding in (0, 1):
                with self.subTest(encoding=encoding):
                    spec.mesh_format.file_type = encoding
                    path = str(Path(directory) / "edited.msh")
                    mshio.save_msh(path, spec)
                    loaded = mshio.load_msh(path)
                    mshio.validate_spec(loaded)
                    np.testing.assert_array_equal(loaded.nodes.entity_blocks[-1].tags, expected_tags)
                    self.assertEqual(loaded.nodes.entity_blocks[-1].data[0], 0.125)
                    np.testing.assert_array_equal(loaded.node_data[0].entries[0].data, [9, 8, 7])
                    np.testing.assert_array_equal(
                        loaded.elements.entity_blocks[0].data,
                        spec.elements.entity_blocks[0].data,
                    )
                    self.assertEqual(loaded.physical_groups[0].name, "edited corner")


if __name__ == "__main__":
    unittest.main()
