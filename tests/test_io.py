#!/usr/bin/env python3

import mshio
import numbers
import pytest
import logging

class TestNodes:
    logger = logging.getLogger(__name__)

    def test_node_block(self):
        block = mshio.NodeBlock()
        assert isinstance(block.entity_dim, numbers.Number)
        assert isinstance(block.entity_tag, numbers.Number)
        assert isinstance(block.parametric, numbers.Number)
        assert isinstance(block.num_nodes_in_block, numbers.Number)
