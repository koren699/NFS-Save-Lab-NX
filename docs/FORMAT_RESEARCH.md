# NFS save-file format research / research log

## Confirmed
- [StevensND/nfsmw-nx](https://github.com/StevensND/nfsmw-nx) is a recompilation of Xbox 360 Most Wanted (2005) and includes readable backup mirror logic (`saves/<profile>/actual` and `anterior`). The active save lives in a separate nested runtime data tree; changing the mirror alone does not restore the active file.
- A raw 32-bit number match is **insufficient evidence** for a Cash, Bounty or Heat offset. Generic raw-byte patching caused a user's game save to become un-loadable. The original was recovered from `.nfslab.bak1`.
- [focanmess/NFSMW_Xbox_Save](https://github.com/focanmess/NFSMW_Xbox_Save) handles the **original Xbox version**, including specific HMAC-SHA1/MD5 fields, and requires a fixed file size. Its layout and signature algorithms are **not validated for Xbox 360**.
- Community tools targeting the PC edition report different structure and hashes; do **not** copy offsets across platforms without samples.

## New result — validated single Xbox 360 recompilation profile (2026-10-13)

A single privately supplied **62688-byte (0xF4E0)** MC02 save was inspected **read-only**. The private file itself has **not been uploaded to this public repository**. The in-game values associated with it were Cash=900 and displayed Bounty=31550.

- Format starts with ASCII `MC02`. File length at offset `0x04` is big-endian `0xF4E0`.
- **EA CRC #1** at `0x10` equals EA's non-reflected CRC32 of `[0x1C,0x24)`.
- **EA CRC #2** at `0x14` equals EA CRC32 of `[0x24,EOF)` (includes the trailing MD5).
- **EA CRC #3** at `0x18` equals EA CRC32 of `[0x00,0x18)` (covers the first two stored CRCs).
- **MD5** at the last 16 bytes equals `MD5([0x34,EOF-16))`.
- All **four stored digests/checksums matched independently recomputed values** for the supplied save. This establishes their verification layout on this sample only, not whether the game accepts a modified copy.
- A unique exact **u32 big-endian 900** is at byte offset `0x4039`: **strong Cash candidate**, not yet independently confirmed by a before/after pair.
- Exact **float32 big-endian 31550** matches at `0x010E`, but no u32 representation matches. **Ambiguous Bounty candidate** only: it might be another game statistic.
- Do **not** enable writing based solely on these matches. Validate with independent, controlled before/after samples, recalculate checksums in correct dependency order, and conduct a gameplay load test on a disposable profile.

The older Xbox Original tool's HMAC-SHA1 save signing scheme is *not* the verified layout above. The sample uses the MC02/EA CRC32/MD5 layout; the same CRC variant appears in EA's published community reverse engineering notes. Other game ports may use different formats.

## Two-save validation (second private user sample, 2026-10-13)

A second 62688-byte MC02 save from the same in-game career was inspected, with displayed **Cash 14400** (previous sample: 900). The raw files remain private; only findings are published.

- Both saves contain the four correctly matching integrity checks (one MD5 and three EA CRC32).
- At the **same offset 0x4039**, the 4-byte big-endian integer changed from `00 00 03 84` (900) to `00 00 38 40` (14400). This was the **only** 32-bit big-endian occurrence of each stated Cash value in its respective save.
- Binary comparison: **40 changed bytes across 13 contiguous ranges**. Changes include Cash, CRC #2 and #3, final MD5, and additional game-state data. CRC #1 remained stable.
- The previously hypothesized Bounty candidate at `0x010E` remained float32 big-endian `31550.0` in both saves, consistent with the user providing no Bounty change; a separate before/after Bounty pair remains necessary.
- Strict research-only script `tools/mc02_cash_test_copy.py` can recompute the verified hashes and produce a new **separate** test copy under explicit opt-in. It never overwrites the original or existing output. An in-game round-trip load of a *patched* test copy is still unverified. No live write option is enabled in the Switch NRO.

### Checksum dependencies on validated MC02 files

```
CRC1 @ 0x10 = EA_CRC32(data[0x1C:0x24])        # stable
MD5  @ EOF-16 = MD5(data[0x34:EOF-16])        # after data changes
CRC2 @ 0x14 = EA_CRC32(data[0x24:EOF])         # includes new MD5
CRC3 @ 0x18 = EA_CRC32(data[0:0x18])           # includes new CRC2
```

**2026-10-13: game-side success confirmed.** A separately produced MC02 copy with Cash **400000** and all 4 recomputed checksums was loaded successfully by the user in the Switch Xbox 360-port, and the game displayed **400000**. This validates a real edited save, not merely the hashes. In-app native Cash editing was then added in v0.5 with a strict file-length, signature, 4-hash and expected-cash gate. It uses the same algorithm, passes byte-identical synthetic cross-checks against the Python reference and preserves a verified .nfslab.bakN. **The first native v0.5 in-app edit itself still needs hardware testing.**

## Not established
- Actual Xbox 360 file internals (record IDs, compression/encryption, money/bounty/heat mapping).
- In-app native v0.5 implementation successfully editing and loading its own Cash modifications (manual Python-equivalent test already succeeded in-game).
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
