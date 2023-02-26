import random
import timeit
from dataclasses import dataclass

import imviz as viz
import numpy as np

from lib import structstore


@dataclass
class Substate:
    subnum: int


@dataclass
class State:
    num: int
    mystr: str
    flag: bool
    substate: Substate


class ExampleGui:
    def __init__(self):
        viz.set_main_window_title('structstore example')

        self.state = structstore.StructStore()
        self.state.num = 5
        self.state.mystr = 'foo'
        self.state.flag = True
        print(self.state.to_yaml())

        self.shmem = structstore.StructStoreShared("/shdata_store", 16384)
        self.shmem.state = State(5, 'foo', True, Substate(42))
        self.shmem.vec = np.array([[1.0, 2.0], [3.0, 4.0]])
        self.num_cnt = 0

        self.shmem2 = structstore.StructStoreShared("/shsettings_store")


    def update_gui(self):
        start = timeit.default_timer()
        if viz.begin_window('State'):
            # viz.autogui(self.state)
            viz.autogui(self.state.deepcopy())
        viz.end_window()
        if viz.begin_window('SharedState'):
            viz.autogui(self.shmem)
            if viz.button('Add number'):
                setattr(self.shmem, f'num{self.num_cnt}', self.num_cnt)
                self.num_cnt += 1
        viz.end_window()
        if viz.begin_window('SharedSettings'):
            viz.autogui(self.shmem2)
        viz.end_window()
        end = timeit.default_timer()
        if random.random() < 0.01:
            print(f'cycle time = {(end - start) * 1000:.1f} ms')

    def run(self):
        while viz.wait(vsync=True):
            self.update_gui()


if __name__ == '__main__':
    gui = ExampleGui()
    gui.run()
