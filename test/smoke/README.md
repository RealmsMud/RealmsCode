# test/smoke

Runtime drivers against a live, freshly-built RealmsCode server; not part of RealmsTests (need real game data and a running process). Run manually.

- `telnet_smoke.py --host H --port P --data ~/realms/realms` -- telnet client for account/login/character creation (reads config.xml for DmPass unless `--dmpass` is given).
- `rest_smoke.sh` -- `sh test/smoke/rest_smoke.sh`; `API_BASE` defaults to `http://localhost:8080`, needs a non-empty `<JwtSecret>` in config.xml.

Requires: server running with `REALMS_BASE_PATH` set, ports 3333/8080 reachable, a test account under the character limit.

Both scripts use unique `Smk`-prefixed names and restore mutated files via `finally`/`trap` on exit, even on failure (`telnet_smoke.py --keep` skips cleanup). Each prints a PASS/FAIL/SKIP table and exits non-zero on any FAIL.
