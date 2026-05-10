# v0.1.5

HLVR hands and config repair release.

## Changed

- Installer now repairs a missing or truncated root `hlvr.cfg` using HLVR default mod settings.
- This restores settings needed for base HLVR behavior, including wrist HUD mode, hand scale, weapon scale, classic-mode state, movement, and weapon handling defaults.
- Hardened the audio guard so an interrupted launch recovers the previous full config before disabling FMOD again.
- Removed old empty aliases for HLVR commands from expansion `autoexec.cfg`; the proxy DLL now handles command filtering so commands like `VModEnable`, `vr_wpnanim`, and `vr_muzzleflash` are not blanked on the client side.

# v0.1.4

HLVR client asset overlay release.

## Changed

- Installer now overlays the base HLVR client-side assets into Opposing Force and Blue Shift after copying expansion content.
- Adds HLVR hand models, VR viewmodels, base weapon models, `animov` models, weapon event scripts, HUD sprites/textures, fonts, `GameUI.dll`, and `particleman.dll` to each expansion folder.
- This prevents stock expansion files from shadowing HLVR hands, weapon handling, weapon selection, and wrist HUD assets while keeping expansion maps, sounds, scripts, and server DLLs intact.

# v0.1.3

Expansion audio guard release.

## Changed

- Generated launchers now set root `hlvr.cfg` to `vr_use_fmod=0` before `hl.exe` starts, so HLVR cannot initialize FMOD before the expansion audio fix applies.
- Launchers now use `start /wait` and restore the previous `hlvr.cfg` after the game exits.
- Installer now copies `HLVR-Expansion-AudioGuard.ps1` beside the launchers.
- Troubleshooting docs now call out `FMOD successfully initialized` as the sign that the setting was applied too late.

# v0.1.2

Opposing Force VR weapon fallback release.

## Changed

- Opposing Force-only weapon and ammo pickups now map to base Half-Life weapon classes that the Steam HLVR client can hold and show in its VR HUD.
- Pipe wrench and knife now fall back to the HLVR crowbar path.
- Desert Eagle, M249, sniper rifle, spore launcher, shock rifle, penguin, and matching ammo pickups now use closest HLVR-supported equivalents.
- Barnacle grapple and Displacer remain stock Opposing Force behavior to avoid breaking campaign progression.
- Docs now describe the fallback mappings and explain that HLFixes is optional for this proxy layer.

# v0.1.1

Documentation clarification release.

## Changed

- README and install guide now clearly state that users must launch through the generated `.bat` files in the Half-Life VR Mod folder.
- Installer completion text now warns not to use Steam's `Change Game` menu for these VR expansion launches.

# v0.1.0

Initial public package for Half-Life VR Mod expansion compatibility.

## Included

- One-click Windows installer.
- Opposing Force proxy DLL.
- Blue Shift proxy DLL.
- HLVR expansion launchers.
- FMOD disablement for expansion audio.
- Official HLFixes installer handoff.
- Step-by-step install and troubleshooting docs.

## Suggested GitHub Repository Description

Installer patch for playing Half-Life: Opposing Force and Blue Shift in the Steam Half-Life VR Mod through generated `.bat` launchers, with proxy DLL fixes for expansion scripting, audio, and HLVR client commands.
