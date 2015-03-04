#!/bin/bash
./coverage.sh
genhtml --output-directory lcov spudlib.info
echo "Open lcov/index.html with your favourite browser"
