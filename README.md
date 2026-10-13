# NFS Save Lab NX v0.9

Nintendo Switch homebrew utility for fan ports of Need for Speed Most Wanted (2005), Underground 2 and Carbon.

## New: Edit Bounty (Most Wanted Xbox 360, MC02)

The user confirmed a manually patched MC02 save with **100,000 in-game Bounty** successfully loaded on Switch. The new native universal editor reuses the verified 4-hash repair and backup process, but auto-discovers valid active pursuit records across all 25 slots. It computes the **authoritative Bounty total** as:

```
total = BE32(save[0xE86D:0xE871])   # sold-car bounty history
      + sum(BE32(record[0x10:0x14]) for live pursuit records)
```

Pursuit records start at `0xE2F5`, stride `0x38`; an empty/retired record starts with handle `0xFF`, even when stale Bounty bytes persist. The previous float32 Bounty cache is **not** authoritative and must not be edited.

**EDIT BOUNTY (MC02)**: enter desired total Bounty yourself. If multiple cars remain, L/R selects which valid active car record receives the difference. The editor attempts to default to the currently selected car based on the career car number, and refuses to reduce below the sum of sold-car history plus every other active car. It validates the structure, all 4 checksums, exact 62,688-byte MC02 file and expected prior value, creates a verified original backup before commit, then rewrites the target record and recalculates all integrity data.

Also included: proven **Edit Cash**, read-only scanning and snapshot comparisons, backup/restore, HD poster frontend.

**Important**: A successfully patched Bounty save was tested in-game, but the new v0.9 native UI and future-sale/multi-car cases have not yet been hardware validated. Before any test, completely close the game and make an independent backup of the whole save folder. Editing Underground 2 and Carbon remains unsupported.

## Artwork installation

The full v0.9 HD edition ships as a **single NRO** with RGB565 336×360 pixel-art posters appended in the NRO. Simply copy it to `sdmc:/switch/nfs_save_lab_nx.nro`. A bare GitHub Actions artifact contains the lower-quality fallback art unless separately packaged.

[Ukrainian user guide](README_UA.md)
