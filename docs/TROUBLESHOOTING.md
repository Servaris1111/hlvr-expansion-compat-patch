# Troubleshooting

## The intro gestures work but there is no dialogue or music

Make sure the launcher includes:

```text
+vr_use_fmod 0
```

Also check the expansion config has:

```text
vr_use_fmod "0"
```

HLVR's FMOD layer can fail to resolve expansion speech/music correctly. The patch intentionally uses GoldSrc audio for Opposing Force and Blue Shift.

## The pipe wrench or other Opposing Force weapons cannot be held in VR

Install v0.1.2 or newer of this patch and launch with `Launch Opposing Force VR.bat`.

The Steam HLVR client does not natively understand several Opposing Force-only weapons. The patched Opposing Force proxy converts those pickups to HLVR-supported base Half-Life weapons when they spawn, so VR hands, weapon controls, health, and ammo HUD behavior stay on the HLVR path.

Expected fallback examples:

- `weapon_pipewrench` -> `weapon_crowbar`
- `weapon_eagle` -> `weapon_357`
- `weapon_m249` -> `weapon_9mmAR`
- `weapon_sniperrifle` -> `weapon_crossbow`
- `weapon_sporelauncher` -> `weapon_rpg`

The barnacle grapple is intentionally left as the stock Opposing Force weapon because maps can require it for progression.

## Opposing Force exits with duplicate cvar errors

The proxy DLL should prevent duplicate cvar registration. Confirm these files exist:

```text
gearbox\dlls\opfor.dll
gearbox\dlls\opfor_stock.dll
```

`opfor.dll` should be the proxy from this patch. `opfor_stock.dll` should be the original Steam Opposing Force DLL copied from the user's own installation.

## Blue Shift exits or ignores the patch

Confirm `bshift\liblist.gam` uses the proxy directly:

```text
gamedll "dlls\hl.dll"
```

Confirm these files exist:

```text
bshift\dlls\hl.dll
bshift\dlls\hl_stock.dll
```

## HLFixes did not install

Run the installer again, or run the official HLFixes installer manually. The patch installer downloads the official `Windows.zip` release and launches `Installer.exe`.

HLFixes is not required for the Opposing Force weapon fallback layer. You can run `Install-HLVRExpansionPatch.ps1 -SkipHLFixes` if you want only the HLVR expansion proxy and launcher files.

## Restore a backup

Look inside:

```text
Half-Life VR Mod\_hlvr_expansion_patch_backup
```

Copy the backed up file back to its original location.
