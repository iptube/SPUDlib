#!/bin/bash
#set -ev
./configure --with-check --enable-gcov --disable-shared
make clean
make check

lcov -d . --no-external --capture --directory src --output-file spudlib.info
if [ "${TRAVIS_PULL_REQUEST:=false}" = "false" ]; then
    genhtml --output-directory lcov spudlib.info
    echo "Open lcov/index.html with your favorite browser"
else
    coveralls-lcov spudlib.info
fi
