# NFS save-file format research / research log

## Confirmed
- [StevensND/nfsmw-nx](https://github.com/StevensND/nfsmw-nx) is a recompilation of Xbox 360 Most Wanted (2005) and includes readable backup mirror logic (`saves/<profile>/actual` and `anterior`). The active save lives in a separate nested runtime data tree; changing the mirror alone does not restore the active file.
- A raw 32-bit number match is **insufficient evidence** for a Cash, Bounty or Heat offset. Generic raw-byte patching caused a user's game save to become un-loadable. The original was recovered from `.nfslab.bak1`.
- [focanmess/NFSMW_Xbox_Save](https://github.com/focanmess/NFSMW_Xbox_Save) handles the **original Xbox version**, including specific HMAC-SHA1/MD5 fields, and requires a fixed file size. Its layout and signature algorithms are **not validated for Xbox 360**.
- Community tools targeting the PC edition report different structure and hashes; do **not** copy offsets across platforms without samples.

## Not established
- Actual Xbox 360 file internals (record IDs, compression/encryption, money/bounty/heat mapping).
- Exact checksum/signature algorithm(s) for the Xbox 360 port's playable profile.
- Whether Heat Level is stored directly in the same save, per car, or in a derived field.
- Compatibility across MW, U2, and Carbon ports.

## Research procedure
1. Work with a disposable save profile and keep independent backups.
2. Capture `before` and `after` from **the same live save path** with exactly one in-game change and known displayed values.
3. Use v0.4 **Compare Snapshots**, which never writes and lists changed ranges and possible direct 32-bit numeric pairs.
4. Inspect binary pairs and save headers outside the Switch as needed; identify and validate checksums across multiple independent pairs.
5. Enable edits only after writing tests that perform a round trip, verify file integrity, and load successfully in-game. Default remains read-only until then.

## Safety invariants in v0.4
- No raw patch function is present in the compiled save scan engine.
- Scanning and comparison are read-only; no `*.bak` is altered by them.
- Manual backup copies only; manual restore demands explicit ZL+ZR+A and first creates a safety backup.
- Test cases verify before/after candidate discovery, scanner correctness, and backup/restore behavior.
