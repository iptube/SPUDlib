#!/bin/bash
./configure --enable-check --enable-gcov --disable-shared
make clean
make check

coveralls --include src -x '.c' --gcov-options '\-lp' -b src
