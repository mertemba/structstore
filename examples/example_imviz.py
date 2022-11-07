import collections
import random
import timeit
from typing import Dict

import imviz as viz
import yaml

from lib import structstore
from lib import structstore_example


class ExampleGui:
    def __init__(self):
        viz.set_main_window_title('example window')

        self.settings = structstore_example.Settings()
        print(type(self.settings))
        print(self.settings.__slots__)
        print(self.settings.num)
        print(self.settings.to_yaml())

        self.state = structstore.StructStore()
        self.state.add_int('num')
        self.state.add_str('mystr')
        self.state.add_bool('flag')
        print(type(self.state))
        print(self.state.__slots__)
        print(self.state.num)
        print(self.state.to_yaml())

        self.shmem = structstore.StructStoreShared("/dyn_shdata_store")
        self.shstate = self.shmem.get_store()
        self.shstate.add_int('num')
        self.shstate.add_str('mystr')
        self.shstate.add_bool('flag')

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
        viz.end_window()
        end = timeit.default_timer()
        if random.random() < 0.01:
            # print(f'settings: {self.settings.to_str()}')
            # print(f'state: {self.state.to_str()}')
            print(f'cycle time = {(end - start) * 1000:.1f} ms')

    def run(self):
        while viz.wait(vsync=True):
            self.update_gui()


if __name__ == '__main__':
    gui = ExampleGui()
    gui.run()
