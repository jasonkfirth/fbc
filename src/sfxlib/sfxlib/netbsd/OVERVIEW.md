# NetBSD Folder Guide

This folder is the preliminary NetBSD target layer for `sfxlib`.

It follows the `gfxlib2` BSD pattern: keep shared Unix support in
`sfxlib/unix/`, and make the NetBSD directory a thin target-specific shim.

## Files In This Folder

### `fb_sfx_netbsd.h`
Small NetBSD backend header.

### `sfx_netbsd.c`
NetBSD target shim.

Job:
- provides background feed scaffolding for unattended playback
- owns the NetBSD driver list
- keeps capture stubbed until a real NetBSD path is added

### `sfx_driver_oss.c`
Preliminary NetBSD playback slot.

Job:
- gives NetBSD a dedicated local driver entry point
- currently fails initialization until real audio backend code is added
