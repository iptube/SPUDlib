#!/bin/bash
#./configure --with-check --enable-gcov --disable-shared
./configure --with-check --enable-gcov
make clean
make check

lcov --directory src --capture --no-external --output-file spudlib.info
genhtml --output-directory lcov spudlib.info
echo "Open lcov/index.html with your favourite browser"
