import unittest
from dataclasses import dataclass
from typing import List, Dict

import numpy as np

import structstore


class TestFailures0(unittest.TestCase):
    def test_failures_0(self):
        state = structstore.StructStore()

        def insert_invalid_key_type():
            state.val = {5: "foo"}

        self.assertRaises(TypeError, insert_invalid_key_type)

        def insert_invalid_field_type():
            state.obj = object()

        self.assertRaises(TypeError, insert_invalid_field_type)

        state.vec = np.ndarray(shape=(0, 2), dtype=np.float64)

        def matrix_to_yaml():
            state.vec.to_yaml()

        self.assertRaises(RuntimeError, matrix_to_yaml)
