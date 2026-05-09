# GoldSrc Proxy DLLs

This folder contains the small Windows proxy DLL used by the installer.

The proxy keeps the original expansion server DLL intact:

- Opposing Force: `gearbox\dlls\opfor.dll` is backed up to `opfor_stock.dll`, then replaced with the proxy.
- Blue Shift: `bshift\dlls\hl.dll` is backed up to `hl_stock.dll`, then replaced with the proxy.

At runtime the proxy loads the stock DLL, forwards entity exports such as `worldspawn`, `ambient_generic`, and `scripted_sequence`, and intercepts HLVR-only client commands before the stock expansion DLL sees them.

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

## Rebuilding

The checked-in binaries are Windows 32-bit DLLs built with llvm-mingw. To rebuild them, install llvm-mingw and point `HLSDK_DIR` at a Half-Life SDK compatible source tree containing `dlls\extdll.h` and `engine\eiface.h`.

```powershell
.\tools\Build-Proxies.ps1 -LlvmMingwBin "C:\path\to\llvm-mingw\bin" -HLSDKDir "C:\path\to\hlsdk"
```
