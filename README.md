# NFS Save Lab NX — v0.6

Nintendo Switch save-file homebrew for fan-made Need for Speed ports. **Pixel-art game posters are now used in the home menu.**

## Install v0.6 (important: copy the art too!)

The graphics pack is supplied **alongside**, not inside the executable. Extract the full release ZIP into the microSD root, preserving the folders:

```text
sdmc:/switch/nfs_save_lab_nx.nro
sdmc:/switch/NFS_Save_Lab_NX/posters/mw.rgb
sdmc:/switch/NFS_Save_Lab_NX/posters/u2.rgb
sdmc:/switch/NFS_Save_Lab_NX/posters/cx.rgb
```

The `.rgb` images are prepared 168×210 RGB24 pixel-art assets. They have been cropped and resized from game posters and appear in the three selection cards. **If you copy only the NRO, it will still launch, but shows an “ART PACK MISSING” fallback instead of posters.**

## Money editing

The v0.5 Most Wanted Xbox 360 MC02 Cash editor is preserved unchanged: exact 62688-byte file, verified MD5+three EA CRCs, verified backup, explicit controller confirmation; 0–999999999 custom amount. Cash editing succeeded on a real in-game save, including a successful v0.5 user hardware test. Bounty and Heat remain read-only pending verified field locations.

**Back up your real career outside the game's save folder before editing.** Close the game first.

## Repository and build

[GitHub Actions](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml) compiles the NRO. Build artifacts contain the NRO only. Download the full **v0.6 bundle** supplied with the release/conversation for the poster assets.

[Українська інструкція](README_UA.md)
