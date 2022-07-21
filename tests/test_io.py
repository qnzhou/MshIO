#!/usr/bin/env python3

import mshio
import numbers
import pytest
import logging
import numpy as np
import sys


class TestNodes:
    logger = logging.getLogger(__name__)

    def generate_tmp(self):
        block = mshio.NodeBlock()
        assert sys.getrefcount(block) == 2
        block.tags = np.array([1, 2, 3])
        assert sys.getrefcount(block) == 2
        tags = block.tags
        assert sys.getrefcount(block) == 3
        assert block.tags[0] == 1
        return tags

    def test_memory_ownership(self):
        tags = self.generate_tmp()
        assert len(tags) == 3
        assert np.all(tags == [1, 2, 3])

        # As a limitation of numpy's dlpack support, tensors are readonly.
        with pytest.raises(ValueError) as e:
            tags.setflags(write=True)

    def test_node_block(self):
        block = mshio.NodeBlock()
        assert isinstance(block.entity_dim, numbers.Number)
        assert isinstance(block.entity_tag, numbers.Number)
        assert isinstance(block.parametric, numbers.Number)
        assert isinstance(block.num_nodes_in_block, numbers.Number)
        assert isinstance(block.tags, np.ndarray)
        assert isinstance(block.data, np.ndarray)

        assert len(block.tags) == 0
        tags = np.array([1, 2, 3])
        block.tags = tags
        assert len(block.tags) == 3
        assert len(block._tags_) == 3
        assert np.all(block.tags == tags)

        assert len(block.data) == 0
        vertices = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]])
        block.data = vertices
        assert len(block.data) == 3
        assert np.all(block.data == vertices)

    def test_element_block(self):
        block = mshio.ElementBlock()
        facets = np.array([[1, 0, 1, 2], [2, 2, 1, 3]])
        block.data = facets

        assert len(block.data) == 2
        assert block.data.shape[1] == 4
        assert np.all(block.data == facets)

    def test_data(self):
        data = mshio.Data()
        data.header.string_tags = ["test"]
        data.header.real_tags = [0.]
        data.header.int_tags = [0, 1, 3, 1]

        data.tags = np.array([1, 2, 3], dtype = int)
        data.data = np.array([1., 2., 3.]).reshape((3, 1))

        assert len(data.tags) == 3
        assert np.all(data.tags == [1, 2, 3])
        assert len(data.data) == 3
        assert np.all(data.data.ravel() == [1., 2., 3.])
