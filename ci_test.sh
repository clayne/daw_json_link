#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug -j 2
ctest
