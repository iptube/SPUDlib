#!/bin/bash
if [ ! -d "build" ]; then
    mkdir build
fi
cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -Dcoveralls=ON -Dcoveralls_send=OFF && make all coveralls && lcov --directory src/CMakeFiles/spud.dir --capture --output-file spudlib.info &&  genhtml --output-directory lcov spudlib.info
echo "Open build/lcov/index.html with your favourite browser"
