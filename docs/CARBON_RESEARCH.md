# NFS Carbon (original Xbox -> Nintendo Switch, nfsc-sw) save research

## First authentic Carbon profile: verified money discovery (2026-10-13)

A private user-provided Xbox original Carbon save ZIP was inspected read-only: user reports **5,000 cash** and a **Chevrolet Camaro**. The actual game save is `ALIAS_...`, 262,236 bytes (`0x4005C`); it is accompanied by `SaveMeta.xbx` (profile-name metadata, 42 bytes) and `saveimage.xbx` (XPR0 image, 18,432 bytes). **Do not commit these personal game saves to the public repository.**

- The main file begins with `20CM` (MC02 signature interpreted as little-endian); its size field at `+0x04` is little-endian `0x0004005C`. This is a **different game/serialization profile** from the previously validated Most Wanted Xbox 360 MC02 save.
- Cash **does not appear anywhere in the raw save as standard u32/u16 or float32/float64, little- or big-endian**, because the relevant data is packed.
- A genuine `JDLZ` v2 compressed block is present at offset **`0x34BC0`**, with little-endian decompressed length `0x8000` (32,768 bytes) and compressed length `0xC18` (3,096 bytes).
- A safe read-only implementation of EA JDLZ decompression produced 32,768 bytes. Inside the **unique** record beginning `2287685084:\t`, the eighth tab-separated field (0-based index **7**, including the record key at index zero) reads **`5000`**, an *exact* match for the reported in-game Cash. This location is also corroborated by `NFS Carbon/Read_Data_Table.cs` (`tabs[7]`) in [TheLastOfLogic/NFS-Carbon-Save-Editor](https://github.com/TheLastOfLogic/NFS-Carbon-Save-Editor), but that tool targets PC / Xbox 360, so its write procedure must not be assumed compatible with Xbox Original.
- The internal `Game` block at `0x7C` has an **exact verified MD5**: 16-byte MD5 stored at `0x6C`, covering `[0x7C, 0x1006C)` (65,520 bytes). It does **not** cover the later JDLZ block.
- The header-like u32 fields at `0x10/0x14/0x18` do **not** match the previously validated Most Wanted Xbox 360 EA CRC32 algorithm when applied to the obvious little-endian length ranges. This is **not** proof of corruption; it means the Xbox Original integrity mechanism still needs separate validation. Do not write/replace checksums by guesswork.
- The bytes after this compressed JDLZ payload are presently zero. This suggests room for a longer compressed block, but **does not prove** that rewriting or shifting it would be accepted. A wrapper/pointer may also store its size. Do not modify until validated.

**Next controlled test:** ask user to keep Chevrolet Camaro, earn or spend a small known amount of money, save and fully exit Carbon, report *exact new balance*, then provide a second untouched archive from the **same** career/profile. Compare compressed content and checksums, and prepare a separately named test copy only after verifying all container lengths/hashes. Until then, Carbon remains **read-only** in the Switch utility.

## Independent second-save validation and v0.11 read-only extractor (2026-10-13)

The same user's second original Xbox Carbon career ZIP reports **10,200 cash** (previously **5,000**). Both original `ALIAS_KOREN69` files are exactly **262,236 bytes** and begin `20CM`; their companion `SaveMeta.xbx` and `saveimage.xbx` files are **byte-identical**, so the change is within the main profile.

The two authentic JDLZ blocks at the **same offset `0x34BC0`** unpack correctly to `0x8000` = 32,768 bytes:

| | 5,000 baseline | 10,200 updated |
|---|---:|---:|
| Main-file length | 262236 | 262236 |
| JDLZ compressed length | 3096 (`0xC18`) | 3269 (`0xCC5`) |
| JDLZ decompressed length | 32768 | 32768 |
| Career money record `2287685084:` tab index 7 | `5000` | `10200` |
| `Game` MD5 at `0x6C`, covering `[0x7C,0x1006C)` | matches | matches |

The JDLZ payload changes in several additional records because gameplay progressed between snapshots. A second occurrence of the new balance is present in an unrelated numeric-statistics line (`2268361667:`) but does **not** establish that the cache needs updating when editing.

### Header integrity blocker

Neither profile's stored four-byte values at `0x10`, `0x14`, `0x18` match straightforward EA CRC32 using the same initial-seed/endian conventions previously verified on Most Wanted Xbox 360 profiles, even though both saves are native game-generated. The `Game` MD5 at `0x6C` is independently valid but covers only a **separate** region, not the compressed Cash record at `0x34BC0`. Do not claim the Carbon wrapper checksum/signature is understood. Do not use the Most Wanted writer, and do not patch the real save until the Xbox Original wrapper can be rebuilt reliably.

### v0.11

A strictly read-only decoder is in `source/carbon.h` and `source/carbon.c`. It validates the `20CM` size, locates exactly one `JDLZ` v2 block, enforces decode bounds to a 32768-byte career table, and retrieves the unique money record's seventh field. The Carbon tools screen exposes `CARBON CASH INFO`. Synthetic JDLZ tests in `tests/test_carbon.c` and `tools/test_carbon_reader.py` exercise both balances and reject broken input. **Write remains disabled**. The privately provided actual saves are not stored in this public repository.

## Verified from port source (2026-10-13)
- Port: https://github.com/antoxa2584x/nfsc-sw
- The original Xbox release is statically recompiled; it is **not** the Xbox 360 edition of Most Wanted and MUST NOT use MC02 Cash/Bounty offsets or hash algorithms.
- `src/main.c` on Switch specifies `NFSU2_DEFAULT_SAVE_DIR "sdmc:/switch/nfscx/save"` and passes it into `xbox_path_init(game_dir, save_dir)`.
- The port's `xboxrecomp/src/kernel/kernel_path.c` maps Xbox `T:\\` to `save/TitleData/`, `U:\\` to `save/UserData/`, and `Z:\\` to `save/Cache/`. It also generates virtual partition images, e.g. `Partition0.img`, which are NOT game save profiles.
- The previous NFS Save Lab NX v0.9 Carbon scanner targeted `nfscx/game/UDATA`; v0.10 now prioritizes `nfscx/save`, recursively scans candidate files, skips `*.img` virtual partitions and displays candidate filename, exact file size and first eight bytes in a **read-only** panel.
- `nfscx/game/UDATA` remains a fallback for nonstandard copies. Do not assume all files found in `save` are editable game profiles.

## Pending validation
- Need user's original, working Carbon career save file(s), preferably from `sdmc:/switch/nfscx/save/` (with all adjacent small metadata files where practical). Keep game **closed** while copying; do not upload personal data unrelated to game saves.
- Determine exact profile path, magic, checksum/signature algorithm, Cash field, and possible checksum metadata stored separately. The original Xbox save may differ from PC/PS2/Xbox 360 Carbon save formats.
- Compare two legitimate in-game Carbon saves with a **single known change** of money (e.g. an unspent reward), record before/after Cash values. Do not use real progress as the edit test until a safe disposable backup was verified.
- Only enable Carbon writes once before/after fields and integrity have passed strict host tests and a disposable in-game load test.

## Safety
**Read-only for Carbon in v0.10.** Existing Most Wanted MC02 Cash/Bounty editing is deliberately limited to Most Wanted. No hex editing of unknown Carbon saves.
