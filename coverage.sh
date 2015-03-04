#!/bin/bash
./configure --with-check --enable-gcov --disable-shared
make clean
make check

lcov -d . --no-external --capture --directory src --output-file spudlib.info
coveralls-lcov spudlib.info

