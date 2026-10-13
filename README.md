# NFS Save Lab NX — v0.5

Nintendo Switch graphical save-file utility for community Need for Speed ports.

**NEW: Verified Most Wanted Xbox 360 MC02 Cash editing.** Two genuine files (Cash 900 and 14400) established a 32-bit big-endian Cash field at `0x4039`. The user confirmed that a separately edited test save with **Cash 400000** loads in-game. The equivalent native Switch code is compiled and regression-tested against the known Python algorithm, but **the new v0.5 Switch UI itself has not yet been hardware-tested**.

The `EDIT CASH (MC02)` operation:
- Is restricted to **Most Wanted Xbox 360**, live save (not a backup mirror), exact 62688-byte MC02 profile with all **4 valid integrity values** (MD5 and EA CRC1–3).
- Automatically reads current Cash and lets you choose **any target 0–999,999,999** using the D-pad; no forced maximum.
- Requires **ZL+ZR+A** after reviewing old and new values.
- Creates a verified `<save>.nfslab.bakN` before writing, produces and verifies the fully hashed result, and preserves recovery files if the process cannot recover cleanly.
- Refuses unknown structures, wrong sums, stale current Cash, and temp-file conflicts.

**Back up your working career independently BEFORE TESTING.** Filesystem/power failures can still damage saves. Close the game when editing. Do not try on a valuable career before a throwaway profile test.

Other features remain: read-only Cash/Bounty/Heat number scans, read-only before/after snapshot comparison, verified manual backup and restore. There is **no** verified edit support yet for Bounty, Heat, Underground 2, or Carbon. `*.header` metadata and the port's `Headers` directory are excluded from the save browser.

## Install and operate

[GitHub Actions builds](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml) → latest green run → Artifacts `NFS-Save-Lab-NX` → unzip `nfs_save_lab_nx.nro` to `sdmc:/switch/`. Run in HBMenu with the game closed.

Home screen L/R selects game → A lists saves → choose **[LIVE]** file → **EDIT CASH (MC02)** → D-pad chooses target number → A reviews → hold ZL+ZR and press A to commit. B returns.

[Українська інструкція](README_UA.md) · [Format research log](docs/FORMAT_RESEARCH.md)
