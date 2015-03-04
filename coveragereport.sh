#!/bin/bash
./coverage.sh
lcov --directory src --capture --output-file spudlib.info
genhtml --output-directory lcov spudlib.info
echo "Open lcov/index.html with your favourite browser"
