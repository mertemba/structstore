from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

ext_modules = [
    Pybind11Extension(
        "structstore",
        ["src/structstore_pybind.cpp"],
        extra_link_args=["-lrt"],
        extra_compile_args=['-std=c++17'],
    ),
]

setup(name="structstore",
      version="0.0.1",
      description="Structured object storage, dynamically typed, to be shared between processes",
      author="Max Mertens",
      author_email="max.mail@dameweb.de",
      license="BSD-3-Clause",
      zip_safe=False,
      python_requires=">=3.6",
      ext_modules=ext_modules,
      include_package_data=True,
      install_requires=[
          "pybind11",
      ])
