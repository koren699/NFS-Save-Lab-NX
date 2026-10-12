# NFS Save Lab NX

**v0.3 — Editable Cash, Bounty, Heat Level and manual Backup / Restore.**

This experimental homebrew scans local save files for community ports of **Need for Speed: Most Wanted (2005)**, **Underground 2** and **Carbon**.

### New in v0.3
After choosing a save, select **Cash**, **Bounty**, or **Heat Level** (1–5). Use the D-pad to enter your own old and new amounts; nothing is forced to the maximum. Choose **Create Backup** to copy the original, or **Restore Backup** to select a saved .nfslab.bakN snapshot and restore it after creating another safety copy.

### What is new in v0.2
- 1280×720 native libnx framebuffer rendering, no heavy GUI dependencies.
- Dark graphite interface with racing lines, neon green/cyan/orange themed game cards.
- Save browser, number entry, byte-match viewer, patch confirmation and result screens.
- Joy-Con/controller navigation: `L/R` game selection; `A` continue; `B` back; D-pad navigate and change digits; `+` exit from home.
- Original save automatically moved to `.nfslab.bakN` before writing raw bytes.

### Download

1. Open [GitHub Actions](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml).
2. Open a successful run and download its **NFS-Save-Lab-NX** artifact (ZIP).
3. Copy `nfs_save_lab_nx.nro` to `sdmc:/switch/` and start it from HBMenu.

**Safety warning:** This is **not** a verified money cheat. Scanning for a 32-bit number does not establish that it is the actual cash field. The application does **not** recalculate game-specific save integrity checksums. Modified saves may not load. Always use a spare save and independent backups. Touch controls are not implemented yet.

[Українська інструкція](README_UA.md)
