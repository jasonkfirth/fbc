# FreeBSD Folder Guide

This folder is the preliminary FreeBSD target layer for `sfxlib`.

Like `gfxlib2`, the idea is to keep shared Unix pieces in `sfxlib/unix/`
and let the target directory override or add only the FreeBSD-specific
parts.

## Files In This Folder

### `fb_sfx_freebsd.h`
Small FreeBSD backend header.

Job:
- exposes the FreeBSD backend state and lifecycle helpers
- keeps the target-local API obvious for later real driver work

### `sfx_freebsd.c`
FreeBSD target shim.

Job:
- provides Linux-style background feed scaffolding for unattended playback
- owns the FreeBSD driver list
- leaves capture as a stub for now

### `sfx_driver_oss.c`
Preliminary FreeBSD OSS driver slot.

Job:
- reserves the target-local playback driver entry point
- currently fails initialization on purpose until real FreeBSD audio code lands

## Design Note

This is intentionally a thin target layer over the shared Unix code,
mirroring the way `gfxlib2` handles BSD targets.
