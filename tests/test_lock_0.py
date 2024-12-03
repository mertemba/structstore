import unittest

import numpy as np

import structstore as stst


class TestLock0(unittest.TestCase):
    def test_lock_0(self):
        state = stst.StructStore()
        with state.write_lock():
            state.foo = stst.StructStore()

        with state.read_lock():
            self.assertRaises(RuntimeError, lambda: setattr(state, 'foo', stst.StructStore()))

    def test_lock_1(self):
        state = stst.StructStore()
        state.lst2 = []
        with state.lst2.write_lock():
            state.lst2.append(stst.StructStore())

        # implicitly get read lock in 'obj'
        obj = state.lst2[0]
        self.assertEqual(str(obj), "{}")
        # write lock cannot be used now
        self.assertRaises(RuntimeError, lambda: state.lst2.write_lock())

        del obj

        # successfully get write lock, as 'obj' is deleted now
        with state.lst2.write_lock():
            pass

        with state.lst2.write_lock():
            # cannot use write lock and read lock at the same time
            self.assertRaises(RuntimeError, lambda: state.lst2.read_lock())

        for subs in state.lst2:
            # iterating implicitly gets a read lock, so write lock is an error here
            self.assertRaises(RuntimeError, lambda: state.lst2.write_lock())
