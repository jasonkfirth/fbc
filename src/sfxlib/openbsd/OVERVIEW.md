# OpenBSD Folder Guide

This folder is the preliminary OpenBSD target layer for `sfxlib`.

It is intentionally thin, matching the `gfxlib2` BSD style where the
shared Unix layer does most of the work and the target directory owns the
small OpenBSD-specific shim.

## Files In This Folder

### `fb_sfx_openbsd.h`
Small OpenBSD backend header.

### `sfx_openbsd.c`
OpenBSD target shim.

Job:
- provides background feed scaffolding for unattended playback
- owns the OpenBSD driver list
- leaves capture and real device hookup for later

### `sfx_driver_oss.c`
Preliminary OpenBSD playback slot.

Job:
- reserves an OpenBSD-local driver entry point
- currently fails initialization until real backend code lands
