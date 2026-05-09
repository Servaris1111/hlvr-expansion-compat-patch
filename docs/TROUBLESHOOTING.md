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

## Restore a backup

Look inside:

```text
Half-Life VR Mod\_hlvr_expansion_patch_backup
```

Copy the backed up file back to its original location.
