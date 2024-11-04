import unittest
from dataclasses import dataclass
from typing import Dict, List

import numpy as np
from _mystruct0_py import Frame

import structstore


class TestMystruct0(unittest.TestCase):
    def test_mystruct_0(self):
        frame = Frame()
        self.assertEqual(frame, frame)
        self.assertEqual(type(frame.copy()), dict)
        state = structstore.StructStore()
        state.frame = Frame()
