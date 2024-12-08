import unittest
from dataclasses import dataclass
from typing import List, Dict

import numpy as np

import structstore


@dataclass(slots=True)
class Substate:
    subnum: int


@dataclass
class State:
    num: int
    value: float
    mystr: str
    flag: bool
    substate: Substate
    lst: List[int]


class TestBasic0(unittest.TestCase):
    def test_basic_0(self):
        state = structstore.StructStore()
        state.num = 5
        state['value'] = 3.14
        state.mystr = "foo"
        state.flag = True
        state.none = None
        state.lst = [1, 2, 3, 5, 8]
        state.tuple = (0.0, 0.0)
        state.vec = np.ndarray(shape=(0, 2), dtype=np.float64)
        state.sub = dict(vec=np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float32))
        state.mat = np.array([[1.0, 2.0], [3.0, 4.0]])
        state.check()
        self.assertNotEqual(state.vec, state.mat)
        self.assertNotEqual(state.vec, state)
        del state['none']
        self.assertFalse(hasattr(state, 'none'))
        del state.flag
        self.assertFalse(hasattr(state, 'flag'))

        # check dict types
        self.assertEqual(type(state.sub), structstore.StructStore)
        self.assertEqual(type(state.copy()), dict)
        self.assertEqual(type(state.copy()["sub"]), structstore.StructStore)
        self.assertEqual(type(state.deepcopy()["sub"]), dict)

        # check matrix types
        self.assertEqual(type(state.mat), structstore.StructStoreMatrix)
        self.assertEqual(str(state.mat), "[1,2,3,4,]")
        state.mat2 = state.mat.copy()
        self.assertEqual(state.mat2, state.mat)

        state_copy: Dict = state.deepcopy()
        print(state_copy["vec"])
        print(state.vec.copy())
        self.assertTrue((state.vec.copy() == state_copy["vec"]).all())
        self.assertTrue(state.sub.vec.copy()[0] == state_copy["sub"]["vec"][0])
        self.assertTrue((state.mat.copy() == state_copy["mat"]).all().all())
        del state_copy["vec"]
        del state_copy["sub"]
        del state_copy["mat"]
        del state_copy["mat2"]
        self.assertEqual(state_copy, {
            "num": 5,
            "value": 3.14,
            "mystr": "foo",
            "lst": [1, 2, 3, 5, 8],
            "tuple": [0, 0],
        })
        state.lst.append(42)
        state.lst.insert(0, 42)
        self.assertEqual(len(state.lst), 7)
        self.assertEqual(state.lst.pop(0), 42)
        state.lst.extend([2, 3, 4])
        self.assertEqual(len(state.lst), 9)

        # check list types
        state.lst2 = []
        state.lst2.append(structstore.StructStore())
        self.assertEqual(type(state.lst2), structstore.StructStoreList)
        self.assertEqual(type(state.lst2.copy()), list)
        self.assertEqual(type(state.lst2.copy()[0]), structstore.StructStore)
        self.assertEqual(type(state.lst2.deepcopy()[0]), dict)
        state.lst2.clear()
        self.assertEqual(state.lst2.copy(), [])
        state.lst3 = state.lst2.copy()
        state.check()

        state2 = structstore.StructStore()
        state2.state = state.deepcopy()
        self.assertEqual(state2.state, state)
        state.sub.vec.copy()[0] += 1.0
        self.assertNotEqual(state2.state, state)
        state2.check()
        state2.clear()
        self.assertTrue(state2.empty())
        self.assertEqual(str(state2), "{}")
