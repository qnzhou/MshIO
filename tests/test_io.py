#!/usr/bin/env python3

import mshio
import numbers
import pytest
import logging
import numpy as np


class TestNodes:
    logger = logging.getLogger(__name__)

    def generate_tmp(self):
        block = mshio.NodeBlock()
        block.tags = np.array([1, 2, 3])
        return block.tags

    def test_tmp(self):
        tags = self.generate_tmp()
        assert len(tags) == 3
        assert tags[0] == 1
        tags.setflags(write=True)
        tags[0] = -1
        assert tags[0] == -1

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
        facets = np.array([
            [1, 0, 1, 2],
            [2, 2, 1, 3]
            ])
        block.data = facets

        assert len(block.data) == 2
        assert block.data.shape[1] == 4
        assert np.all(block.data == facets)
