#!/bin/sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/engine.c tests/test_engine.c -o tests/test_engine
./tests/test_engine
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/backup.c tests/test_backup.c -o tests/test_backup
./tests/test_backup
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/diagnostics.c tests/test_diagnostics.c -o tests/test_diagnostics
./tests/test_diagnostics
rm -f tests/test_engine tests/test_backup tests/test_diagnostics
