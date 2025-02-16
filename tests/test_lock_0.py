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
        # one thread can get one write lock (explicitly) ...
        with state.lst2.write_lock():
            # ... and a second one (implicitly during append) at the same time
            state.lst2.append(stst.StructStore())

        # this does not create a read lock anymore
        obj = state.lst2[0]
        self.assertEqual(str(obj), "{}")
        # write lock can still be used now
        state.lst2.write_lock()

        del obj

        # double iter is ok
        for subs in state.lst2:
            for subs2 in state.lst2:
                print(subs2)

    def test_lock_2(self):
        state = stst.StructStore()
        state.lst2 = [stst.StructStore()]

        with state.lst2.write_lock():
            # cannot use write lock and read lock at the same time
            self.assertRaises(RuntimeError, lambda: state.lst2.read_lock())

        with state.lst2.read_lock():
            # cannot use write lock and read lock at the same time
            self.assertRaises(RuntimeError, lambda: state.lst2.write_lock())

        for subs in state.lst2:
            # iterating implicitly gets a read lock, so write lock is an error here
            self.assertRaises(RuntimeError, lambda: state.lst2.write_lock())

        def append_while_iter():
            try:
                for subs in state.lst2:
                    # iterating implicitly gets a read lock, so append is an error here
                    state.lst2.append(5)
            except RuntimeError as e:
                # additionally check error message
                self.assertEqual(str(e), "timeout while getting write lock")
                raise

        self.assertRaises(RuntimeError, append_while_iter)

        def pop_while_iter():
            for subs in state.lst2:
                # iterating implicitly gets a read lock, so pop is an error here
                state.lst2.pop(0)

        self.assertRaises(RuntimeError, pop_while_iter)

        def extend_by_self():
            try:
                state.lst2.extend(state.lst2)
            except RuntimeError as e:
                # additionally check error message
                self.assertEqual(str(e), "trying to acquire read lock while current thread has write lock")
                raise

        # extending by itself would need a read lock and a write lock
        self.assertRaises(RuntimeError, extend_by_self)
