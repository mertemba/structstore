import unittest

import numpy as np

import structstore


class TestContainers(unittest.TestCase):
    def test_list(self):
        store = structstore.StructStore()
        lst = structstore.StructStoreList()
        store.lst = lst
        # this is allowed, but a no-op
        store.lst = store.lst

        lst.append(42)

        def copy_nonempty_list():
            try:
                store.lst = lst
            except RuntimeError as e:
                self.assertEqual(
                    str(e), "copy assignment of structstore::List is not supported"
                )
                raise

        self.assertRaises(RuntimeError, copy_nonempty_list)

        # out-of-bounds access
        self.assertRaises(IndexError, lambda: lst[42])
        self.assertRaises(IndexError, lambda: lst.insert(42, 'foo'))
        self.assertRaises(IndexError, lambda: lst.pop(42))

    def test_matrix(self):
        store = structstore.StructStore()
        store.mat = np.array([1.0, 2.0])

        shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
        shmem.mat = store.mat
        shmem.mat = store.mat
        shmem.check()
