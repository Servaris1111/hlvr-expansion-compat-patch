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
