# NFS Save Lab NX

Experimental save-file tool for the Nintendo Switch homebrew ports of **Need for Speed: Most Wanted (2005)**, **Underground 2** and **Carbon**.

**Build status:** [GitHub Actions](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml) — automated compilation with `devkitpro/devkita64`.

### Download the NRO

1. Open **[Actions → Build Nintendo Switch NRO](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/build.yml)**.
2. Open the latest successful (green) run.
3. Under **Artifacts**, select `NFS-Save-Lab-NX`.
4. Unzip and copy `nfs_save_lab_nx.nro` to `sdmc:/switch/NFS_Save_Lab_NX/`.

For detailed instructions, see [README_UA.md](README_UA.md).

**Important:** The initial release is an experimental scanner/patcher, **not a verified cash cheat**. It cannot regenerate the game's proprietary save integrity data. Always back up saves and test on a spare profile; a modified file may fail to load.
