# DragonFly Folder Guide

This folder is the preliminary DragonFly BSD target layer for `sfxlib`.

As in `gfxlib2`, the shared Unix layer remains the base and the target
directory provides a narrow DragonFly-specific shim.

## Files In This Folder

### `fb_sfx_dragonfly.h`
Small DragonFly backend header.

### `sfx_dragonfly.c`
DragonFly target shim.

Job:
- provides background feed scaffolding for unattended playback
- owns the DragonFly driver list
- leaves capture and concrete driver hookup for future work

### `sfx_driver_oss.c`
Preliminary DragonFly playback slot.

Job:
- reserves a DragonFly-local driver entry point
- currently fails initialization until real backend code is written
