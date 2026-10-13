# Underground 2 (Original Xbox → Switch) — Cash research

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

### NOT YET VERIFIED IN GAME: Cash field

The user reports **125** in-game Cash. One and only one occurrence of the 32-bit little-endian integer `125` (`7D 00 00 00`) appears at **`0xA19A`** in the authenticated main file. The 16-bit little-endian integer 125 also appears only there. This is a strong candidate but cannot be declared authoritative without comparing a second save with changed Cash or loading a test patch.

An independent Python test created an **isolated candidate** with `LE32[0xA19A]=100000`, repaired all three HMAC tags in the required order (tag 1 → tag 2 → tag 3), verified all three signatures, and verified that **no bytes outside the candidate field and signature area have changed**. The exact file length remains 53,868 bytes. It was provided privately as `NFSU2_Xbox_Cash_125_to_100000_TEST_ONLY.zip`; neither save nor candidate is committed to GitHub. **User has not yet reported whether the game accepts it.**

Do not enable the Underground 2 editor in NFS Save Lab NX until we know the game loads the patched file AND the Cash displayed in the game is 100000. If it fails, request a second native save after changing Cash (in-game) to isolate the true field, or inspect additional integrity structures.

### Safety

Every experiment is done on a separately named candidate from an original copy; the running game must be closed, the user must back up the complete career folder, and restore the original after failures. The verified Carbon and Most Wanted editing paths should not be modified until U2 validation passes.
