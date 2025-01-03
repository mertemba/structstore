import unittest
from dataclasses import dataclass
from typing import Dict, List

import numpy as np
from _mystruct0_py import Frame, Track

import structstore


class TestMystruct0(unittest.TestCase):
    def test_mystruct_0(self):
        frame = Frame()
        frame.t = 2.5
        self.assertEqual(frame, frame)
        self.assertEqual(type(frame.copy()), dict)
        state = structstore.StructStore()
        state.frame = frame
        self.assertEqual(state.frame.t, 2.5)
        state.frame2 = state.frame.copy()
        state.frame2.t = 3.0
        self.assertEqual(state.frame.t, 2.5)
        state.check()

        self.assertEqual(state.frame.t_ptr, 2.5)
        state.frame.t_ptr += 10.0
        self.assertEqual(state.frame.t, 12.5)

        track = Track()
        self.assertEqual(track.frame1, track.frame2)
        state.track = track
        state.track.frame2 = state.frame
        self.assertNotEqual(state.track.frame1, state.track.frame2)
        state.track.frame_ptr = state.frame
        self.assertEqual(type(state.track.frame_ptr), Frame)
        self.assertEqual(type(state.track.frame_ptr.copy()), dict)
        self.assertEqual(state.track.frame_ptr, state.frame)
        state.check()
