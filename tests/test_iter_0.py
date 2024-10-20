import unittest

import numpy as np

import structstore as stst


class TestIter0(unittest.TestCase):
    def test_iter_0(self):
        state = stst.StructStore()
        state.lst = [0, 1, 2, 3]
        for i, num in enumerate(state.lst):
            self.assertEqual(num, i)
        state.lst2 = []
        state.lst2.append(stst.StructStore())
        with state.lst2.lock():
            for subs in state.lst2:
                self.assertEqual(str(subs), "{}")
