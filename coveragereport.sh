#!/bin/bash
if [ ! -d "build" ]; then
    mkdir build
fi
cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -Dcoveralls=ON -Dcoveralls_send=OFF && make all coveralls coverage_report
