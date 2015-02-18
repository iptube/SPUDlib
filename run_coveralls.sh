#!/bin/bash

./configure --with-check --enable-gcov && make && make check
coveralls --exclude-pattern ".+?\.h" --exclude-pattern "\w+/test" --gcov-options '\-lp'

