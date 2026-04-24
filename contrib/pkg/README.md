# FreeBASIC distro package metadata

This directory contains distro-native packaging metadata for building
FreeBASIC source packages. These templates are intentionally kept under
`contrib/pkg` so they do not interfere with the main build scripts or the older
binary-tarball packaging helpers in `contrib/deb` and `contrib/rpm`.

The package version should match `FBVERSION` in `mk/version.mk`.

The templates assume a source archive named like:

```text
freebasic-1.20.1.tar.xz
```

Before submitting to a distro repository, replace placeholder checksums with
the actual source archive checksum and adjust distro policy details such as
maintainer names, changelogs, accepted architectures, and split packages.

## Included templates

- `rpm/freebasic.spec` for RPM-based distros.
- `arch/PKGBUILD` for Arch Linux and derivatives.
- `alpine/APKBUILD` for Alpine Linux.
- `gentoo/dev-lang/freebasic/freebasic-1.20.1.ebuild` for Gentoo overlays.
- `void/template` for Void Linux.

The source-build flow used by the templates is:

```sh
make bootstrap-minimal
make all FBC=bootstrap/fbc
make install DESTDIR="$pkgdir" prefix=/usr
```

Some distros use `gmake`/`emake` wrappers around GNU make, but the build stages
are the same.
