# FreeBASIC 1.20.4 documentation

This directory collects the documentation added or substantially revised for
the 1.20.2 through 1.20.4 development cycle. The generated language manual
remains under `doc/manual`; these guides cover release, package, port, and
experimental runtime subjects that do not fit naturally in the language
reference.

## Start here

- [1.20.4 release notes](release-1.20.4.md) summarize the user-visible work
  completed since the 1.20.2 development baseline.
- [Package repository and installation](packages.md) explains the packages at
  `deb.fbxl.net`, installation choices, target packages, and checksums.
- [NuttX and RISC-V](nuttx.md) describes the QEMU, ESP32-P4, and RP2350-PiZero
  development workflows.
- [Portable serial control](serial.md) documents the `fbcom.bi` API layered on
  top of `OPEN COM`.
- [gfxlib3](gfxlib3/README.md) describes the opt-in render-threaded GPU graphics
  runtime and links to its architecture, parity, performance, and verification
  records.
- [sfxlib examples](../examples/sfxlib/README.md) introduces generated sound,
  sample playback, MIDI, output capture, diagnostics, and stress examples.

The complete per-change record is in [changelog.txt](../changelog.txt). The
generated manual is the authoritative reference for established language
keywords and the built-in graphics and sound command syntax.

## Platform guides

- [AROS](aros.md)
- [RISC OS](riscos.md)
- [Windows CE](wince.md)
- [Linux MIPS](mips.md)
- [NuttX](nuttx.md)

Cross-target example results and known external-library limitations are kept
beside the examples:

- [JavaScript, Android, and Xbox notes](../examples/cross-target-notes.md)
- [Cygwin notes](../examples/cygwin-example-notes.md)
- [Windows notes](../examples/windows-example-notes.md)

## Build and qualification records

- [Cross-build matrix](../build_scripts/cross-build-matrix/README.md)
- [Emulated native matrix](../build_scripts/emulated-native-matrix/README.md)
- [gfxlib3 test plan](gfxlib3/test-plan.md)
- [gfxlib3 verification record](gfxlib3/verification.md)

The build records are evidence for maintainers. They are not a promise that
every optional target or external dependency is available on every host.

## Building the generated manual

Build the FreeBASIC documentation tools first, then generate every manual
format from the checked-in Wakka cache:

```sh
make -C doc
make -C doc/manual clean
make -C doc/manual all
```

The complete build requires `zip`, `gzip`, and Microsoft's HTML Help Compiler.
Pass the compiler location as `HHC=/path/to/hhc` if it is not on `PATH`. The
resulting versioned CHM, HTML, text, fbhelp, and Wakka archives are written to
`doc/manual/`. A release build is not complete until each archive exists and
the generated `html/00index.html` links to the new manual pages.

<!-- end of README.md -->
