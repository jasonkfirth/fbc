# Package repository and installation

The packages built for this development branch are published at
[deb.fbxl.net](https://deb.fbxl.net/). This is an unofficial, experimental
package repository. The upstream FreeBASIC downloads remain at SourceForge and
the upstream project remains at [freebasic.net](https://freebasic.net/).

## Universal installer

On a supported Unix-like host, download the dispatcher, inspect it if desired,
and run it with the system shell:

```sh
curl -fsSLO https://deb.fbxl.net/install.sh
sh install.sh
```

The dispatcher detects Debian, Ubuntu, Raspbian, Alpine, postmarketOS, Fedora,
AlmaLinux, Rocky Linux, openSUSE, Slackware, the BSD family, Haiku, illumos,
Solaris, macOS, and Cygwin. `FREEBASIC_INSTALL_TARGET` can override detection,
and `FREEBASIC_REPO_URL` can point the installer at a staging mirror.

The distro-specific scripts accept `--release`, `--arch`, and `--base-url` when
an automatic value is unsuitable. They also accept `--skip-repo`,
`--skip-install`, and `--skip-verify` for package maintenance work.

The actively qualified Linux lanes for this release are Ubuntu Resolute,
Debian Trixie and Sid, Raspbian Trixie, current postmarketOS, Rocky Linux 10,
AlmaLinux 10, openSUSE Leap 16.0, and openSUSE Tumbleweed. Other retained files
remain downloadable, but their presence does not imply current qualification.

## Package groups

On Debian and Ubuntu, `freebasic-full` installs the normal compiler plus the
bindings, examples, documentation, and the target packages recommended for
that distribution. Android remains a separately selected package. Smaller
installations can select individual packages:

- `freebasic` contains the host compiler.
- `freebasic-runtime` and `freebasic-dev` contain runtime libraries and public
  development files.
- `freebasic-bindings`, `freebasic-examples`, and `freebasic-doc` contain their
  named data sets.
- `freebasic-js`, `freebasic-android`, `freebasic-wii`, and `freebasic-nuttx`
  add optional target SDKs. Xbox packages are published beside the Resolute
  amd64 packages and under the Windows Xbox directory.

Windows standalone archives and installers are grouped under `mingw32/`,
`mingw32-js/`, `mingw32-android/`, `mingw32-wii/`, and `mingw32-xbox/`. DOS,
BSD, Haiku, illumos, Solaris, macOS, Cygwin, AROS, RISC OS, and Windows CE have
separate top-level directories when a package or cross-target output exists.

Always use the repository directory listing or `repo.json` to discover the
current file. Some target families deliberately retain earlier packages, and
not every host-side package is rebuilt in every release.

## Android development on an Android phone

The Termux bootstrap uses a native ARM64 Ubuntu PRoot for the FreeBASIC host
compiler and Termux's native Clang, NDK sysroot, AAPT2, D8, Java, and APK signer.
It does not install QEMU or x86-64 NDK host binaries.

In Termux:

```sh
pkg install curl
curl -fLO https://deb.fbxl.net/install/termux-ubuntu-android-bootstrap.sh
chmod 700 termux-ubuntu-android-bootstrap.sh
./termux-ubuntu-android-bootstrap.sh
```

The normal run provisions the PRoot and builds an `arm64-v8a` smoke-test APK.
Use `--setup-only` to provision without building the smoke test. The script
verifies the SHA-256 of every downloaded compiler, target package, and Android
platform jar before extracting it.

## Integrity and release identity

Each package directory contains a `SHA256SUMS` file. The repository root also
publishes `release-1.20.4-SHA256SUMS`, which records the complete release file
set, and `repo.json`, which records the release number, source commit, package
families, tested platforms, and generation time.

For a downloaded file:

```sh
sha256sum -c SHA256SUMS
```

Run the check from the directory containing both the download and its matching
`SHA256SUMS` entry. Do not mix a checksum file from one package directory with
a similarly named package from another distribution or architecture.

<!-- end of packages.md -->
