# HLVR Expansion Compat Patch

Installer patch for playing **Half-Life: Opposing Force** and **Half-Life: Blue Shift** through the Steam **Half-Life VR Mod**.

This project packages the compatibility work needed to make the expansions launch through HLVR without breaking the original expansion cutscenes, dialogue, music, and scripted sequences.

## What It Fixes

- Adds Opposing Force and Blue Shift launchers for the Steam Half-Life VR Mod.
- Copies expansion content from a legally installed Steam Half-Life folder into the HLVR folder.
- Uses the HLVR client DLL for VR rendering/input in both expansions.
- Installs a tiny server-DLL proxy for each expansion:
  - Opposing Force: `opfor.dll` forwards to `opfor_stock.dll`.
  - Blue Shift: `hl.dll` forwards to `hl_stock.dll`.
- Filters HLVR-only commands that stock expansion DLLs do not understand.
- Prevents duplicate cvar registration startup failures.
- Forces expansion audio to use normal GoldSrc audio instead of HLVR FMOD, fixing missing intro music and missing NPC dialogue.
- Optionally launches the official HLFixes installer.

## Download and Install

See [docs/INSTALL.md](docs/INSTALL.md).

Quick version:

1. Close Half-Life/HLVR.
2. Download and extract this repo.
3. Double-click `HLVR-Expansion-Patch-Installer.cmd`.
4. Launch from:
   - `Launch Opposing Force VR.bat`
   - `Launch Blue Shift VR.bat`

## Why This Exists

The Steam HLVR client works well with base Half-Life, but the expansions use different stock game DLLs. The current Steam HLVR build sends VR-specific client commands and uses an FMOD sound hook that the stock expansion DLL/audio paths were not built for.

The proxy approach keeps the original expansion game logic intact. That matters for scripted scenes like the Opposing Force helicopter intro, where replacing the server DLL can break gestures, speech, or music.

## What Is Not Included

This repository does **not** include Valve, Gearbox, Steam, Half-Life, Opposing Force, Blue Shift, or Half-Life VR Mod game content. The installer copies required expansion files from the user's own Steam installation.

HLFixes is not redistributed here. The installer downloads and launches the official HLFixes Windows release.

## Repository Contents

- `HLVR-Expansion-Patch-Installer.cmd` - double-click installer launcher.
- `installer/Install-HLVRExpansionPatch.ps1` - main installer.
- `bin/opfor/opfor.dll` - Opposing Force proxy DLL.
- `bin/bshift/hl.dll` - Blue Shift proxy DLL.
- `src/goldsrc-proxy` - proxy source and export definition files.
- `tools/Build-Proxies.ps1` - rebuild helper for maintainers.
- `docs` - install and troubleshooting docs.

## Credits

- Valve, Gearbox, and the Half-Life VR Mod team for the games/mods this patches.
- IntriguingTiles for HLFixes.

This project is an unofficial compatibility patch.
