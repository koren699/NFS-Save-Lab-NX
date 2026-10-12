#!/bin/sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/engine.c tests/test_engine.c -o tests/test_engine
./tests/test_engine
rm -f tests/test_engine
