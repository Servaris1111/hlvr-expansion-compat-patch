# Troubleshooting

## The intro gestures work but there is no dialogue or music

Make sure the launcher includes:

```text
+vr_use_fmod 0
```

If `qconsole.log` says `FMOD successfully initialized` before `execing autoexec.cfg`, the setting was applied too late. Install v0.1.3 or newer and relaunch from the generated batch file. The new launchers temporarily write `vr_use_fmod=0` into root `hlvr.cfg` before `hl.exe` starts, then restore the previous file after you exit.

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

The barnacle grapple and Displacer are intentionally left as stock Opposing Force weapons because maps can require them for progression.

## Hands, wrist HUD, or weapon handling do not look like base HLVR

Install v0.1.5 or newer and relaunch from the generated batch file.

GoldSrc resolves assets from the active game folder first. If `gearbox` or `bshift` still contains stock expansion viewmodels or HUD files, those files can shadow the HLVR hands, weapon models, weapon event scripts, and HUD sprites even when the HLVR `client.dll` is loaded. The v0.1.4 installer overlays the base HLVR client-side assets into both expansion folders.

Also check root `hlvr.cfg` in the Half-Life VR Mod folder. It should be a full config file, not a one-line file. Important entries include:

```text
vr_hud_mode=2
vr_gordon_hand_scale=1.0
vr_weaponscale=1.0
vr_classic_mode=0
```

The v0.1.5 installer repairs a missing or truncated `hlvr.cfg` with HLVR defaults. It also removes old empty aliases for `VModEnable`, `vr_wpnanim`, `vr_muzzleflash`, and related HLVR commands from expansion `autoexec.cfg`; those commands are filtered by the proxy DLL instead.

Confirm these files exist and match the base HLVR `valve` folder:

```text
gearbox\models\v_hand_hevsuit.mdl
gearbox\models\vr_hand_hevsuit.mdl
gearbox\models\animov\v_crowbar.mdl
gearbox\sprites\hud.txt
gearbox\events\crowbar.sc
bshift\models\v_hand_hevsuit.mdl
bshift\models\vr_hand_hevsuit.mdl
```

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
