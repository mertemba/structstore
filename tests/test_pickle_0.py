import pickle
import unittest

import numpy as np

import structstore


class TestPickle0(unittest.TestCase):
    def test_pickle_0(self):
        state = structstore.StructStore()
        state.num = 5
        state.value = 3.14
        state.mystr = "foo"
        state.flag = True
        state.none = None
        state.lst = [1, 2, 3, 5, 8]
        state.tuple = (0.0, 0.0)
        
        data = pickle.dumps(state)
        state2 = pickle.loads(data)
        print(state2)
        assert state == state2
        assert state.deepcopy() == state2.deepcopy()
