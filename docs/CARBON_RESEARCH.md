# NFS Carbon (original Xbox -> Nintendo Switch, nfsc-sw) save research

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
