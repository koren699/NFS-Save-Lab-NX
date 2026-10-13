# Underground 2 (Original Xbox → Switch) — Cash research

## Game-tested Cash offset, native editor in v0.16 (2026-10-13)

**Confirmed on the user's real Nintendo Switch:** the isolated patch from **125 → 100,000 Cash** loaded successfully in Underground 2, and the user verified that the money edit works. The authentic career layout is 53,868-byte **Original Xbox 20CM**, with LE32 Cash at **0xA19A** and all three HMAC-SHA1 tags signed using the game-tested **16 zero-byte key**. This is no longer just a candidate field.

**v0.16** introduces `source/u2.c` / `source/u2.h`, with:
- Strict length/magic/header checks: `0xD26C` bytes; `20CM`; LE32 length fields `8` and `0xD218`.
- Re-verification of all 3 HMAC-SHA1 signatures on the existing career before any change.
- Explicit expected-current Cash comparison to detect changes since selecting the profile.
- Write only LE32 `0xA19A`; recompute the three HMACs in the correct order, then verify the entire output and that no unrelated byte was modified.
- `lab_make_backup` for a verified `.nfslab.bakN`; temporary output verification and promotion with an `.nfslab.u2.oldpart` recovery path.
- Clean Underground 2 profile filtering in the UI: no Xbox metadata files, single-profile short route, custom Cash value with `ZL+ZR+A` confirmation.
- Synthetic native C tests and independent Python HMAC fixtures in `tests/test_u2.c` and `tools/test_u2_writer.py`.

The game-side acceptance of the **standalone signed patch** is confirmed, but the **automated write through the newly built v0.16 NRO** still needs first-hardware validation. Keep an independent original save-folder backup before the initial test.

## Authentic original Xbox career profile (2026-10-13)

The user provided a private ZIP containing one real save file, `KOREN69` (original Xbox profile, 53,868 bytes, hex `0xD26C`), plus `SaveMeta.xbx` (30 bytes) and `saveimage.xbx` (4,096 bytes). The reported in-game Cash is **125**. **Do not upload or commit the private save to this repository.**

Source of Switch port: [antoxa2584x/nfsu2-sw](https://github.com/antoxa2584x/nfsu2-sw) (original Xbox static recompilation). Its saves are stored in the extracted game's `game/UDATA` directories.

The authenticated career file begins `20CM`; unlike Carbon's 262236-byte `20CM`, this format contains an **uncompressed** 53784-byte data region. The 16-byte header data are LE32 `{total_size=0xD26C, header_region=8, data_region=0xD218}`. The full file length equals `0x4C + 8 + 0xD218` exactly.

### VERIFIED: three Original Xbox HMAC-SHA1 signatures

The existing Carbon signing algorithm works **exactly** on this original Underground 2 save. Three independent 20-byte tags are stored at:

| Tag | Stored offset | Input data covered | Key |
|---|---|---|---|
| 1 | `0x10..0x23` | `[0x4C, 0x4C + LE32(0x08))` (8 bytes) | 16 zero bytes |
| 2 | `0x24..0x37` | `[0x4C + LE32(0x08), EOF)` (53784 bytes) | 16 zero bytes |
| 3 | `0x38..0x4B` | `[0, 0x38)` (56 bytes) | 16 zero bytes |

These HMACs match **all 60 signature bytes** of the original file. The checksum/signature format and exact ranges are established, so it is not necessary to use the original Xbox title-specific XBE key to check this particular save.

### Original Cash-field discovery (subsequently confirmed in game)

The user reports **125** in-game Cash. One and only one occurrence of the 32-bit little-endian integer `125` (`7D 00 00 00`) appears at **`0xA19A`** in the authenticated main file. The 16-bit little-endian integer 125 also appears only there. This is a strong candidate but cannot be declared authoritative without comparing a second save with changed Cash or loading a test patch.

An independent Python test created an **isolated candidate** with `LE32[0xA19A]=100000`, repaired all three HMAC tags in the required order (tag 1 → tag 2 → tag 3), verified all three signatures, and verified that **no bytes outside the candidate field and signature area have changed**. The exact file length remains 53,868 bytes. It was provided privately as `NFSU2_Xbox_Cash_125_to_100000_TEST_ONLY.zip`; neither save nor candidate is committed to GitHub. **The user has now confirmed that this patched save was accepted in game and the change worked.**

The standalone cash field and signature repair are now proven in game. The v0.16 automatic writer adds verified backups and strict matching; it still needs first-hardware validation as a complete utility.

### Safety

Every experiment is done on a separately named candidate from an original copy; the running game must be closed, the user must back up the complete career folder, and restore the original after failures. The verified Carbon and Most Wanted editing paths should not be modified until U2 validation passes.
