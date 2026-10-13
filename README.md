# NFS Save Lab NX — v0.4 SAFE DIAGNOSTIC BUILD

A homebrew Nintendo Switch tool for examining saves from community **Need for Speed: Most Wanted (2005, Xbox 360)**, **Underground 2** and **Carbon** ports.

## Important safety change in v0.4
The unsafe generic raw-byte write function has been **removed from the application and from the save scan engine**, not merely hidden. Cash, Bounty, and Heat are now **read-only number scans** until a specific save format and its integrity rules are verified.

Available operations after selecting a file:
1. **CASH SCAN** — find possible 32-bit occurrences of your current in-game cash amount without altering the file.
2. **BOUNTY SCAN** — same for a current bounty amount.
3. **HEAT SCAN** — examine current 1–5 value; matches are not proof of a heat field.
4. **COMPARE SNAPSHOTS** — select an existing `.nfslab.bakN` of the *same file*, then choose Cash/Bounty/Heat with L/R and enter the correct earlier and current displayed amounts. Compares both file versions in memory, lists changed byte counts and candidate 32-bit offsets, and never modifies either file.
5. **CREATE BACKUP** — copy a save to `<file>.nfslab.bakN`; verified copy, original left alone.
6. **RESTORE BACKUP** — restore a selected snapshot after creating a *new safety snapshot* of the current save. This is the only operation that replaces a live save and requires **ZL + ZR + A**.

**Not yet supported:** verified edits of Cash, Bounty or Heat, automatic checksum repair, automatic detection of proprietary/encrypted/compressed game save structures. The old Xbox save-format algorithm is **not** assumed to apply to the Xbox 360 port.

## How to collect a diagnostic pair safely
Use a **throwaway profile**, **not your valuable career**, and make a separate copy of the SD card save folder on your computer.
1. Close the game and in the utility select its live save file; select **CREATE BACKUP**.
2. Play the throwaway profile, change only *one* clearly known value (e.g. cash via one race or purchase), and save; close the game.
3. Open the same file in NFS Save Lab NX → **COMPARE SNAPSHOTS**. Choose the earlier backup and the metric, then enter the *before* and *after* numbers.
4. Look at the diagnostic results. Any offsets are **research candidates only**, never confirmed money locations.

Comparison only works when you have an earlier `.nfslab.bakN` file beside the matching live file, and both copies are the same size (32 B–8 MiB). If the port uses a new save file path between runs, compare cannot proceed.

## Install
From [GitHub Actions](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml) open the latest successful run → download the **NFS-Save-Lab-NX** artifact ZIP → copy `nfs_save_lab_nx.nro` directly into `sdmc:/switch/` → open with Homebrew Menu.

**Navigation:** L/R switch games on the home screen; A selects; B goes back; D-pad selects rows and adjusts numbers by digit; + exits from the home screen. In compare snapshot selection, L/R changes Cash/Bounty/Heat. Touch not yet implemented.

Most Wanted live and mirrored save directories depend on its installation; the port generally stores live save content in nested folders under `sdmc:/switch/nfsmw/nfsmw/` and optional readable mirrors under `sdmc:/switch/nfsmw/saves/<profile>/actual` and `anterior`. **The mirror is not the live save.**

**Build checks:** native C tests for read-only scanner, read-only before/after diff and verified backup/restore, followed by a libnx/devkitA64 .nro build. Actual hardware and game save compatibility are **not yet verified**.

[Українська інструкція](README_UA.md) · [Save format research](docs/FORMAT_RESEARCH.md)
