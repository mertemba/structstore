! test -z "$srcdir"

builddir="$srcdir/build"
venvdir="$builddir/venv"

cmake_options=""
if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_PYTHON=ON"
fi
if [ ! -z "$BUILD_WITH_TESTING" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_TESTING=ON"
fi
if [ ! -z "$BUILD_WITH_COVERAGE" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_COVERAGE=ON"
fi
if [ ! -z "$BUILD_WITH_SANITIZER" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_SANITIZER=ON"
fi
