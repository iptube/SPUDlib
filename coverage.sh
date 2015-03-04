#!/bin/bash
./configure --with-check --enable-gcov --disable-shared
make clean
make check

coveralls --exclude-pattern "\w+\.h" --gcov-options '\-lp' -b src


