import random
import timeit
from dataclasses import dataclass

import imviz as viz

from lib import structstore
from lib import structstore_example
from lib import structstore_utils


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

        self.settings = structstore_example.Settings()
        print(self.settings.to_yaml())

        self.state = structstore.StructStore()
        self.state.add_int('num')
        self.state.add_str('mystr')
        self.state.add_bool('flag')
        print(self.state.to_yaml())

        self.shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
        self.shstate = self.shmem.get_store()
        shstate = State(5, 'foo', True, Substate(42))
        structstore_utils.construct_from_obj(self.shstate, shstate)
        self.num_cnt = 0

        self.shmem2 = structstore_example.SettingsShared("/shdata_store")
        self.shsettings = self.shmem2.get_store()


    def update_gui(self):
        start = timeit.default_timer()
        if viz.begin_window('Settings'):
            # viz.autogui(self.settings)
            viz.autogui(self.settings.to_dict())
        viz.end_window()
        if viz.begin_window('State'):
            # viz.autogui(self.state)
            viz.autogui(self.state.to_dict())
        viz.end_window()
        if viz.begin_window('SharedState'):
            viz.autogui(self.shstate)
            if viz.button('Add number'):
                self.shstate.add_int(f'num{self.num_cnt}')
                self.num_cnt += 1
        viz.end_window()
        if viz.begin_window('SharedSettings'):
            viz.autogui(self.shsettings)
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
