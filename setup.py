import sys, re, os

try:
    from skbuild import setup
except ImportError:
    print(
        "The preferred way to invoke 'setup.py' is via pip, as in 'pip "
        "install .'. If you wish to run the setup script directly, you must "
        "first install the build dependencies listed in pyproject.toml!",
        file=sys.stderr,
    )
    raise

setup(
    name="mshio",
    version="0.0.1",
    author="Qingnan Zhou",
    author_email="qnzhou@adobe.com",
    description="A tiny C++ library to read/write ASCII/binary MSH format files.",
    url="https://github.com/qnzhou/MshIO",
    license="Apache-2.0",
    packages=["mshio"],
    package_dir={"": "python"},
    cmake_install_dir="python/mshio",
    include_package_data=True,
    cmake_args=[
        "-DCMAKE_BUILD_TYPE=Release",
        "-DMSHIO_BUILD_TESTS=Off",
        "-DMSHIO_BUILD_EXAMPLES=Off",
        "-DMSHIO_EXT_NANOSPLINE=Off",
        "-DMSHIO_PYTHON=On",
    ],
    python_requires=">=3.8",
)
