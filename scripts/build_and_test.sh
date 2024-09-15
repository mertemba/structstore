#!/bin/bash -ex

BUILD_WITH_TESTING=1 ./scripts/build.sh
cd /tmp/structstore_build
ctest --output-on-failure
