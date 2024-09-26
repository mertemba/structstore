pkgdir="/tmp/structstore_build"

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