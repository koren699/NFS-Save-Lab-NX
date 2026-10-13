#!/bin/sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/engine.c tests/test_engine.c -o tests/test_engine
./tests/test_engine
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/backup.c tests/test_backup.c -o tests/test_backup
./tests/test_backup
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/diagnostics.c tests/test_diagnostics.c -o tests/test_diagnostics
./tests/test_diagnostics
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/mc02.c source/backup.c tests/test_mc02.c -o tests/test_mc02
python3 tools/test_cash_test_copy.py
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/mc02.c source/backup.c tests/test_bounty.c -o tests/test_bounty
python3 tools/test_bounty_reader.py
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/carbon.c tests/test_carbon.c -o tests/test_carbon
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/carbon.c tests/test_carbon_edit.c -o tests/test_carbon_edit
cc -std=c11 -Wall -Wextra -Werror -O2 -Isource source/carbon_auth.c tests/test_carbon_auth.c -o tests/test_carbon_auth
./tests/test_carbon_auth
rm -f tests/test_carbon_auth
python3 tools/test_carbon_reader.py
rm -f tests/test_carbon_edit
rm -f tests/test_carbon
rm -f tests/test_engine tests/test_backup tests/test_diagnostics tests/test_mc02 tests/test_bounty
