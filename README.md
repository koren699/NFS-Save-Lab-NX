# NFS Save Lab NX
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/cca7e0d3-93f6-459f-b33b-841e46ce62a5" />

**Nintendo Switch homebrew save editor** for the fan-made Switch ports of **Need for Speed: Most Wanted (2005)**, **Underground 2**, and **Carbon**.

**Current version: v0.16** · Tested on a real Nintendo Switch with all three games.


[🇺🇦 Українська](README_UA.md) · [🇬🇧 English](README.md) · [🇷🇺 Русский](README_RU.md)



![NFS Save Lab NX v0.16 home screen with Most Wanted, Underground 2 and Carbon pixel-art covers](https://github.com/koren699/NFS-Save-Lab-NX/releases/download/v0.16/01-main-menu.jpg)

<details>
<summary>See more screenshots: Most Wanted, Underground 2 and Carbon</summary>

![Most Wanted — save file selection](https://github.com/koren699/NFS-Save-Lab-NX/releases/download/v0.16/02-most-wanted-save-list.jpg)

![Underground 2 — career selection](https://github.com/koren699/NFS-Save-Lab-NX/releases/download/v0.16/03-underground-2-profiles.jpg)

![Carbon — cash amount and valid Xbox save signatures](https://github.com/koren699/NFS-Save-Lab-NX/releases/download/v0.16/04-carbon-cash.jpg)

</details>


## Supported games

| Game / Switch port | Edit Cash | Edit Bounty | Save format |
|---|---|---|---|
| Need for Speed: Most Wanted (2005) | ✅ | ✅ | Xbox 360 MC02 |
| Need for Speed: Underground 2 | ✅ | — | Original Xbox 20CM |
| Need for Speed: Carbon | ✅ | — | Original Xbox 20CM |

This utility targets the **specific community Switch ports** of the games, not the Windows/PC releases or other console editions. Compatibility with every future port version or every save is not guaranteed.

## Features

- Select a game using one of three high-quality pixel-art poster cards.
- Automatically detect supported career save files and display the current balance.
- Set a **custom Cash amount** for all three games.
- Edit accumulated **Bounty** in Most Wanted. Includes active-car and sold-car history accounting; use **L/R** to choose the active car when several are present.
- Verify the correct game-specific integrity data (MC02 MD5/EA CRC32 for Most Wanted; three HMAC-SHA1 signatures for the Original Xbox games).
- **Create a verified `.nfslab.bakN` backup before modifying a save**, verify the output and provide recovery paths.
- One `.nro` with pixel-art artwork and Homebrew Menu icon in the **full HD release package**.
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/550ffebc-4ad2-4e0b-ac69-7c2aa9cf9667" />
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/65fff7ed-3711-4022-a969-7a829deb1d7e" />
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/752e203b-6b3f-4ac0-aefb-df88e6644d9c" />

## Install and use

1. Download the **full HD** `nfs_save_lab_nx.nro` from [Releases](https://github.com/koren699/NFS-Save-Lab-NX/releases) once the release is published.
2. Copy it to `sdmc:/switch/nfs_save_lab_nx.nro` on your Switch microSD card.
3. **Close the game completely**, then make an independent backup of the *entire game save folder* outside the active game directory.
4. Launch **NFS Save Lab NX** through Homebrew Menu.
5. Choose a game → career save → **Edit Cash**, or **Edit Bounty** in Most Wanted.
6. Set your desired amount with the directional buttons, press **A**, and confirm using **ZL + ZR + A**.
7. Load your career in the game and check the result. Keep the original backup until you confirm successful loading.

> **Important:** The full HD `.nro` includes the custom icon and three high-resolution posters. A standard automatic GitHub Actions build may contain only fallback artwork. Download the release attachment labeled *Full HD*.

## Technical information

- **Most Wanted Xbox 360:** 62,688-byte MC02 profiles, verified MD5 + EA CRC32 integrity.
- **Underground 2 Original Xbox:** 53,868-byte 20CM profiles, Cash stored as a little-endian integer and signed with 3 × HMAC-SHA1.
- **Carbon Original Xbox:** 262,236-byte 20CM profiles with a compact JDLZ career data block and 3 × HMAC-SHA1.
- An unsupported or invalid save is rejected instead of being blindly patched.

All three games' Cash editing and Most Wanted Bounty editing have been confirmed in-game by the project's tester. Backups remain essential.

## Documentation

- [Ukrainian guide](README_UA.md) · [Russian guide](README_RU.md)
- [v0.16 release notes in three languages](docs/RELEASE_NOTES_v0.16.md)
- [Most Wanted format research](docs/FORMAT_RESEARCH.md)
- [Carbon research](docs/CARBON_RESEARCH.md)
- [Underground 2 research](docs/UNDERGROUND2_RESEARCH.md)

Community-made, unofficial homebrew. **Not affiliated with or endorsed by Electronic Arts, Nintendo, or the Need for Speed publishers.** You are responsible for backing up your own save data.
