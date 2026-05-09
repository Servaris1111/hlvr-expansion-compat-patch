# GoldSrc Proxy DLLs

This folder contains the small Windows proxy DLL used by the installer.

The proxy keeps the original expansion server DLL intact:

- Opposing Force: `gearbox\dlls\opfor.dll` is backed up to `opfor_stock.dll`, then replaced with the proxy.
- Blue Shift: `bshift\dlls\hl.dll` is backed up to `hl_stock.dll`, then replaced with the proxy.

At runtime the proxy loads the stock DLL, forwards entity exports such as `worldspawn`, `ambient_generic`, and `scripted_sequence`, and intercepts HLVR-only client commands before the stock expansion DLL sees them.

The Opposing Force build also owns selected OpFor-only weapon/ammo exports and reclassifies them as base Half-Life equivalents before calling the stock entity factory. This keeps unsupported expansion pickups on HLVR's known weapon, input, and HUD paths. The barnacle grapple is forwarded to stock OpFor because campaign maps can depend on it.

The intercepted commands are:

- `VModEnable`
- `vrupd_hmd`
- `vrupdctrl`
- `vr_flashlight`
- `vr_teleporter`
- `vr_anlgfire`
- `vr_lngjump`
- `vr_restartmap`
- `vr_wpnanim`
- `vr_muzzleflash`
- `vrtele`
- `vrspeech`

The proxy also wraps cvar registration so duplicate HLVR/GoldSrc cvars do not abort expansion startup.

## Opposing Force weapon fallbacks

- `weapon_pipewrench`, `weapon_knife` -> `weapon_crowbar`
- `weapon_eagle` and `ammo_eagleclip` -> `weapon_357` and `ammo_357`
- `weapon_m249` and `ammo_556` -> `weapon_9mmAR` and `ammo_9mmAR`
- `weapon_sniperrifle` and `ammo_762` -> `weapon_crossbow` and `ammo_crossbow`
- `weapon_sporelauncher` and `ammo_spore` -> `weapon_rpg` and `ammo_rpgclip`
- `weapon_shockrifle`, `weapon_shockroach` -> `weapon_gauss`
- `weapon_displacer` -> `weapon_egon`
- `weapon_penguin` -> `weapon_snark`

## Rebuilding

The checked-in binaries are Windows 32-bit DLLs built with llvm-mingw. To rebuild them, install llvm-mingw and point `HLSDK_DIR` at a Half-Life SDK compatible source tree containing `dlls\extdll.h` and `engine\eiface.h`.

```powershell
.\tools\Build-Proxies.ps1 -LlvmMingwBin "C:\path\to\llvm-mingw\bin" -HLSDKDir "C:\path\to\hlsdk"
```
