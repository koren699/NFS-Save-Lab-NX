# v0.16 Full HD binary upload

**Before publishing** upload exactly one compiled, hardware-tested binary to this directory, named:

`nfs_save_lab_nx.nro`

Direct upload: [GitHub upload page for this folder](https://github.com/koren699/NFS-Save-Lab-NX/upload/main/release-assets/v0.16).

Use the **FULL HD** NRO from the release kit. The publishing workflow checks:
- NRO magic, ASET metadata, and a real JPEG Homebrew Menu icon;
- all three embedded 336×360 HD pixel posters (`NFSLABP8` trailer);
- the four screenshots in `docs/screenshots`.

After both uploads, run [Publish Full HD Release](https://github.com/koren699/NFS-Save-Lab-NX/actions/workflows/publish-release.yml) and leave version `v0.16`. The workflow creates/updates a public GitHub Release containing the NRO, packaged ZIP, all four screenshots and the Ukrainian/English/Russian release description.

**Do not upload any personal game-save files.** The NRO is an executable utility, not game content. This staging directory is for the release asset only. Note that adding a binary to Git tracks it in repository history; for future versions consider using a release asset directly or a dedicated artifact store instead.
