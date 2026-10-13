# NFS Save Lab NX v0.7

**Single-file homebrew:** Most Wanted, Underground 2 and Carbon pixel-art posters are now **embedded directly into `nfs_save_lab_nx.nro`**. The external poster resource folder from v0.6 is no longer needed.

Place `nfs_save_lab_nx.nro` directly into `sdmc:/switch/` and launch with HBMenu.

## Verified Most Wanted MC02 Cash

Native Cash editing has already been successfully tested in-game with four repaired integrity checksums (MD5 + EA CRC1/2/3). Only validated 62688-byte MC02 saves are permitted. Original is backed up to `.nfslab.bakN`. Always close the game and independently back up the active save.

## New in v0.7: dynamic Bounty detector (READ ONLY)

Two independently supplied MC02 saves reported **Bounty 31550 (float32 BE @ 0x010E)** and **45150 (float32 BE @ 0x015A)**. The record moved, but a 12-byte preceding marker is identical and unique in both saves. The program uses this marker to find and display the current Bounty instead of assuming a fixed offset. Navigate Most Wanted → [LIVE] save → **BOUNTY INFO (MC02)**.

**Bounty modification remains disabled until a patched test save is accepted by the game.** Heat and other NFS ports remain diagnostic/read-only.

## Building

A Python 3 standard-library script (`tools/generate_poster_header.py`) processes `assets/posters_embedded.json` into a generated `source/poster_embedded.h` while `make` runs. The packed images are 80×100 indexed/palette art and are scaled by the custom pixel renderer on Switch. The end-user installation requires one NRO file, with no runtime network access or extra artwork.

[GitHub Actions builds](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml) · [Українська інструкція](README_UA.md)
