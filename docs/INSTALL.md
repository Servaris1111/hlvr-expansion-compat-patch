# Step-by-Step Install

## Requirements

- Steam Half-Life installed.
- Steam Half-Life VR Mod installed.
- Opposing Force and/or Blue Shift content present in the Steam Half-Life folder.
- Windows.

## One-click install

1. Close Half-Life and Half-Life VR Mod.
2. Download this repository as a ZIP and extract it.
3. Double-click `HLVR-Expansion-Patch-Installer.cmd`.
4. If the official HLFixes installer opens, choose Install or Reinstall.
5. Open your **Half-Life VR Mod** install folder.
6. Launch the expansion from the batch files created there:
   - `Launch Opposing Force VR.bat`
   - `Launch Blue Shift VR.bat`

Do not launch Opposing Force or Blue Shift VR through Steam's `Change Game` menu. These batch files are required because they start HLVR with the correct expansion folder, client DLL, first map, and expansion-safe audio settings.

## Manual path install

If Steam is installed in an unusual location:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\Install-HLVRExpansionPatch.ps1 `
  -HalfLifePath "E:\SteamLibrary\steamapps\common\Half-Life" `
  -HLVRPath "E:\SteamLibrary\steamapps\common\Half-Life VR Mod"
```

## Dry run

To see what the installer would do without changing files:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\Install-HLVRExpansionPatch.ps1 -DryRun
```

## What the installer changes

- Copies `gearbox`, `gearbox_hd`, `bshift`, and `bshift_hd` from the Steam Half-Life folder into the Half-Life VR Mod folder.
- Copies the HLVR `client.dll` into each expansion `cl_dlls` folder.
- Copies HLVR action bindings into each expansion.
- Installs the Opposing Force proxy DLL and keeps the original as `opfor_stock.dll`.
- Installs the Blue Shift proxy DLL and keeps the original as `hl_stock.dll`.
- Converts unsupported Opposing Force weapon/ammo pickups to base Half-Life classes the HLVR client can hold and display.
- Forces `vr_use_fmod "0"` for expansion launches so dialogue and music use the normal GoldSrc audio path.
- Creates `Launch Opposing Force VR.bat` and `Launch Blue Shift VR.bat` in the Half-Life VR Mod folder.
- Downloads and runs the official HLFixes Windows installer unless HLFixes already appears installed.

HLFixes is optional. If you only want the HLVR expansion proxy/launcher patch, run the PowerShell installer with `-SkipHLFixes`.

## Backups

Overwritten files are backed up under:

```text
Half-Life VR Mod\_hlvr_expansion_patch_backup\YYYYMMDD-HHMMSS
```
