## Successful live v0.9 Bounty editor test — sale + three purchases (2026-10-13)

The user reports **successful in-game verification** after using the native NFS Save Lab NX v0.9 `EDIT BOUNTY` feature. Importantly, the test was **not** limited to the earlier one-car profile:

1. Sold the old career car.
2. Bought **three new cars**.
3. Used the Switch homebrew UI to set total Bounty to **200,000**.
4. The game loaded the edited save and displayed **200,000** Bounty.

This is a direct success report for the native multi-car editor and its sold-car-history aggregation, selected-record update, and integrity-repair path **in this tested career scenario**. It gives stronger evidence than the earlier manually patched one-car test. It does not establish universal compatibility with every NFS Most Wanted save variant; retain format checks, fail-closed occupancy validation, verified backup, and recovery procedures.

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

## Verified Bounty record relocation (third private user save, 2026-10-13)

The user supplied an updated MC02 career with visible **Bounty 45150** (previous reference **31550**). Both original files are 62688-byte valid MC02 profiles, matching all four original integrity checks. Money in the new save is also different (multiple gameplay changes), so this is *not* a single-variable mutation.

- Old `31550.0` occurs as float32 BE `46 F6 7C 00` at `0x010E`.
- New `45150.0` occurs as float32 BE `47 30 5E 00` at `0x015A`.
- Both values are preceded by the **same unique 12-byte marker** `47 1C B2 90 8B E4 E2 55 47 33 00 31`.
- The record appears to relocate or reorder; avoid hard-coded Bounty offsets.
- v0.7 uses this exact marker to *read* Bounty. It does not modify the value.
- **The Bounty editing path remains unvalidated**: only a copied, separately patched file tested in-game could establish that modifying this float and the integrity values is accepted by the port.

The individual private source saves are not stored in this public repo.

## Corrected Xbox 360 Bounty aggregation — one active car (2026-10-13)

**Important correction:** the prior conclusion that two nonzero pursuit record values must mean two cars currently owned was wrong. The user currently has **one** car, and the binary confirms one live slot.

Detailed private-save byte check (no private save contents published):

- Xbox 360 profile's 25 pursuit records start at `0xE2F5`, stride `0x38`. In the original older snapshot, handles `0` and `1` were both live. In more recent snapshots, slot 0's handle becomes **`0xFF`**, meaning **empty / retired slot**, even though its stale bytes still contain `22550` at `0xE305`. Slot 1 remains live, handle `0x01`.
- After that retirement, **sold-car history** Bounty is present separately as big-endian `u32=22550` at `0xE86D`, not at the stale `0xE305`. This closely matches the PC save format's sold history offset `0xE865` shifted by 8 bytes for this Xbox 360 profile. **Do not count the stale slot 0 bounty and sold-car history twice.**
- The one live car's bounty is big-endian `u32` at **`0xE33D`**. In the earlier one-car profile it was `9000`, and in the new one-car profile it is `22600`.
- Thus the correct *current* rating formula for this save: **sold-history 22550 + live car 22600 = 45150**. The previously displayed `45150.0` float32 at `0x015A` is a **cached/derived value**, not the game's authoritative rating field; editing only this float made the tool report 100000 but did not change the in-game rating.
- New experimental test copy (kept private; not placed in this public repo) changes only `0xE33D` big-endian u32 from `22600` to `77450`, retains sold-history `22550` and the unmodified float cache, then recomputes MD5 and all three EA CRCs. Independent validation of the copy's integrity passes, but **game-side acceptance and actual 100000 total have not yet been checked.**
- Crucial editor safety requirement: validate live pursuit slot occupancy using the handle, differentiate sold-history from stale empty slots, and reject ambiguous layouts. **Keep Edit Bounty disabled until the corrected test is game-tested.**

Related PC format research: [garage_records.py](https://github.com/sprintstate/nfs-mw-save-editor/blob/main/nfs_mw_save_editor/core/garage_records.py) and [rap_sheet_totals.py](https://github.com/sprintstate/nfs-mw-save-editor/blob/main/nfs_mw_save_editor/core/rap_sheet_totals.py). The Xbox profile differs in endianness, size and offsets, but the byte-level examples above support this aggregation model.

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
