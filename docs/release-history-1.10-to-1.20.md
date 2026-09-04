<!--
    Project: FreeBASIC documentation
    --------------------------------

    File: release-history-1.10-to-1.20.md

    Purpose:

        Summarize the complete development path from the 1.10.0 release
        through the current 1.20.4 tree.

    Responsibilities:

        - record the real release and development-version boundaries
        - group user-visible work by version and subsystem
        - direct readers to detailed reference pages and the changelog

    This file intentionally does NOT contain:

        - a duplicate of every changelog entry
        - package filenames that change as repositories are refreshed
        - claims that an untagged development version was a formal release
-->

# FreeBASIC 1.10 to 1.20 development history

This guide covers the complete repository history after the FreeBASIC 1.10.0
release through the documented 1.20.4 tree. The audit examined all 804 commits
from tag `1.10.0` (exclusive) through commit
`26fe670a9aecf2d6eeb5547f01b906a34795160e` (inclusive), including merged
maintenance branches and the FBXL 1.10.4 development import.

The exact change-by-change record is in [changelog.txt](../changelog.txt). This
guide explains the sequence and points to the reference material for features
that need more than a changelog sentence.

## Version boundaries

| Version | Repository boundary | Status |
| --- | --- | --- |
| 1.10.0 | tag `1.10.0`, 2023-05-14 | Released baseline for this audit |
| 1.10.1 | tag `1.10.1`, 2023-12-24 | Maintenance release |
| 1.10.2 | tag `1.10.2`, 2023-12-28 | Maintenance release |
| 1.10.3 | tag `1.10.3`, 2025-04-21 | Maintenance release |
| 1.20.0 | development began at `487b809b4`, 2023-05-20 | Main-line development version; no release tag in this repository |
| 1.20.1 | FBXL integration at `9d28b9587`, 2026-04-23 | Development/package version; no release tag in this repository |
| 1.20.2 | `21e1110a2` through `f14d893cb`, 2026-07-01 to 2026-08-11 | Development baseline; its changes were carried into the 1.20.3 notes |
| 1.20.3 | tag `v1.20.3`, release commit `d60206520`, 2026-08-26 | Tagged FBXL release |
| 1.20.4 | development began at `86c6a62ac`, 2026-08-25 | Current documented tree |

The 1.10 maintenance branch and 1.20 main line developed in parallel. This is
why some 1.20.0 work predates the 1.10.1 through 1.10.3 tags. The later
1.10.4 FBXL branch was imported into 1.20.1 rather than published here as a
separate 1.10.4 tag.

## 1.10 maintenance releases

### 1.10.1

The first maintenance release corrected GCC array descriptors, GAS64 static
debug data and NaN comparisons, OpenGL scaling reset, several preprocessor
macro-context failures, and Emscripten archive, WSTRING, path, and Asyncify
behavior. It added the `-fbgfx` linker-selection option, expanded compile-time
macro helpers, added the MariaDB 3.3.1 binding, and refreshed PostgreSQL,
SQLite, curl, BASS, and MinGW release-tool coverage.

### 1.10.2

The second maintenance release added ARM hard-float CPU and architecture
selection, configurable default CPU types, ARM bootstrap-distribution targets,
and package-target naming. It also updated the BASS binding and made the release
scripts select the correct ARM bootstrap package and options.

### 1.10.3

The third maintenance release fixed UDT pointer `OPERATOR CAST` generation,
DOS wide-character termination, array reallocation diagnostics on newer GCC,
and several package-build problems. It refreshed WinAPI and other bindings and
completed the 1.10-series ARM and BSD release-script work that the 1.20 line
later inherited.

## 1.20.0 compiler and language work

The long 1.20.0 development period concentrated on compiler behavior and
language tooling:

- floating-point comparisons, `CBOOL`, and conditional expressions now handle
  NaN consistently across the GCC, LLVM, GAS, GAS64, and SSE paths;
- fixed-length `STRING * N` storage, initialization, swapping, assignment,
  argument copyback, `LEN`, `LSET`, and `RSET` were made consistent and bounded;
- `#ELSEIFDEF`, `#ELSEIFNDEF`, `#pragma private`, `#pragma profile`, and
  `END WHILE` were added;
- the built-in profiler, `-profgen`, `__FB_PROFILE__`, and
  `__FB_OPTION_PROFILE__` were added;
- `-earraydims`, `-sysroot`, `-gen clang`, `-fpu neon`, `-z nobuiltins`,
  `-z optabstract`, `-nolib`, and fork-identification support were added;
- `__FB_ARG_LISTEXPAND__`, extended `__FB_QUERY_SYMBOL__` queries, runtime error
  constants in `fberror.bi`, and GCC/Clang declarations in `builtin.bi` became
  available;
- Android compiler support, ARM architecture aliases, position-independent
  executable support, and new RISC-V, s390x, and LoongArch identities began
  the architecture expansion;
- the FLTK 1.3 C++ binding added a broad desktop widget, window, menu, image,
  text-control, file-chooser, OpenGL, and printing interface; and
- fixed-length strings, UDT operators with array parameters, bounds checking,
  C++ ABI handling, Unicode source decoding, and GAS64 code generation received
  extensive correctness fixes.

The manual has dedicated pages for the new directives, options, profiler,
intrinsic defines, `builtin.bi`, and fixed-length string rules.

## 1.20.1 platform and library expansion

The 1.20.1 work integrated the FBXL branch and broadened the project from a
small desktop/DOS target set into a package-tested multi-platform tree.

### Language and runtime interfaces

- sfxlib added generated sound, PLAY strings, samples, music, MIDI, capture,
  device selection, raw PCM output, and portable audio backends;
- `OPEN TCP`, `OPEN TCP SERVER`, `TCP ACCEPT`, and `EOC` added client/server
  stream networking, with `fbnetwire.bi` helpers for fixed-width wire values;
- `GETXPAD` added modern controller polling and the touch API added contact
  counting, coordinates, hit testing, and mouse fallback;
- `#pragma gui`, `FB_NO_GFXLIB`, and `FB_NO_SFXLIB` added source-level control
  over subsystem and legacy command surfaces;
- `builtin.bi`, `sfxlib_raw.bi`, portable CRT bindings, and updated external
  bindings made new compiler and library functionality directly usable; and
- `vbdos.bi` exposed the VBDOS-compatible interrupt and register interface for
  legacy programs that require it.

### Targets

- Android gained APK creation, native activity lifecycle, graphics, touch,
  keyboard, audio, assets, threading detection, and real-device launch;
- JavaScript/Emscripten gained a separate compiler package, browser application
  helper, graphics, WebAudio, Asyncify-aware input and sleep, and browser-driven
  tests;
- the original Xbox target was revived on nxdk with XBE/XISO packaging,
  filesystem, threading, controller, graphics, sound, and network support;
- Wii homebrew gained DOL and bundle generation, libogc runtime services,
  graphics, Wiimote input, audio, filesystem, threading, and networking;
- Haiku, Darwin/macOS, FreeBSD, OpenBSD, NetBSD, DragonFly BSD, Solaris, and
  illumos received native runtime, graphics, sound, toolchain, package, or test
  work; and
- Linux ARM, AArch64, PowerPC, PowerPC64, RISC-V 64, s390x, and LoongArch64
  package paths exposed ABI, linker, alignment, and `setjmp` assumptions that
  were corrected in shared code.

### Build and qualification

The build system was split into reusable configuration, platform-feature,
toolchain, source-selection, archive, bootstrap, and install modules. Package
builders and clean-install tests were added for Debian/Ubuntu/Raspbian,
Alpine/postmarketOS, rpm distributions, Slackware, Arch, BSD, Haiku, illumos,
Solaris, Windows/MSYS2, Cygwin, DOS, macOS, Android, JavaScript, Wii, and Xbox.
The fbctests and Exampleageddon runners grew target classification, emulation,
remote execution, deterministic logs, command sweeps, and retained evidence.

## 1.20.2 development baseline

The tree reported version 1.20.2 from July 1 through August 11, 2026. There is
no 1.20.2 tag in this repository, so these are development-build milestones,
not claims about a formal release.

- the `nuttx-riscv32` target, RISC-V 32-bit compiler identity, NuttX runtime,
  socket and GPIO bindings, low-memory graphics, sound glue, QEMU tests, and
  ESP32-P4/RP2350 workflows were introduced;
- macOS gained complete socket, pthread, regex, scheduler, time, stat, iconv,
  libffi, UUID, curl, curses, framework-link, and ownership-focused binding
  tests;
- Unix, BSD, Haiku, Solaris, and illumos headers and runtime paths were
  completed or consolidated for sockets, curses, terminal, package, graphics,
  and sound behavior;
- JavaScript gained direct Emscripten WebSocket declarations and a browser
  example;
- strict GCC and Clang builds, Cppcheck, Clang-Tidy, ShellCheck, fblint, broad
  fbctests, and Exampleageddon passes fixed compiler, library, tool, and example
  warnings and exposed real bounds, ownership, alignment, and lifetime bugs;
- gfxlib3 arrived as an opt-in Vulkan/OpenGL/OpenGL ES graphics runtime, gained
  checked PNG support, and filled historical SCREEN modes 3 through 6; and
- sfxlib added the software FM MIDI fallback and continued portability,
  command-surface, shutdown, and buffer correctness work.

The detailed entries are retained under the 1.20.3 changelog because that was
the next tagged release.

## 1.20.3 tagged release

The tagged 1.20.3 release carried the 1.20.2 baseline forward and completed a
second large portability pass:

- RISC OS, AROS x86-64/ARM/m68k, Linux MIPS32/MIPS64 in both byte orders, and
  Windows CE ARM/MIPS became compiler and runtime targets;
- target-specific compiler work moved into Android, AROS, Cygwin, Darwin, DOS,
  DragonFly, FreeBSD, Haiku, illumos, JavaScript, Linux, NetBSD, NuttX,
  OpenBSD, RISC OS, Solaris, Wii, Win32, Windows CE, and Xbox directories;
- `fbcom.bi` added portable serial status, queue, modem-line, break, and purge
  controls over `OPEN COM` handles;
- GitHub Actions began qualifying native, cross, packaged, and emulated targets,
  publishing tagged artifacts, and retaining failure evidence;
- an ARM64 Termux plus Ubuntu PRoot bootstrap proved APK development directly
  on an Android phone without x86-64 host binaries or QEMU; and
- compiler parsing, fixed-string operations, preprocessor lifetime, TCP,
  filesystem, threading, graphics, sound, package installation, and emulator
  behavior received the fixes recorded in the 1.20.3 changelog.

## 1.20.4 current tree

The current tree adds PNG and SIMD media paths, hardens first-party C and C++
code under whole-tree diagnostics, completes Windows 95 compatibility and
qualification, and improves AROS, RISC OS, NetBSD, Haiku, package publication,
and repository installation. See the focused
[1.20.4 release notes](release-1.20.4.md).

## Reference map

| Subject | Primary documentation |
| --- | --- |
| Compiler options and intrinsic defines | Generated manual, Compiler Options and Intrinsic Defines chapters |
| Fixed-length strings, arrays, UDTs, preprocessor, profiler | Generated language and programmer reference |
| TCP, serial, and portable wire values | Generated `OPEN TCP`, `TCP ACCEPT`, `EOC`, `OPEN COM`, `fbcom.bi`, and `fbnetwire.bi` pages |
| gfxlib2 and gfxlib3 | Generated Graphics chapters and [gfxlib3 guide](gfxlib3/README.md) |
| sfxlib and optional advanced headers | Generated Sound Library Reference and the [sfxlib example guide](../examples/sfxlib/README.md) |
| Android, JavaScript, Wii, Xbox, DOS, and native builds | Generated developer build pages and package-specific manual pages |
| NuttX, AROS, RISC OS, Windows CE, and MIPS | [NuttX](nuttx.md), [AROS](aros.md), [RISC OS](riscos.md), [Windows CE](wince.md), and [MIPS](mips.md) guides |
| Packages and repository integrity | [Package repository and installation](packages.md) |
| Every individual fix and build change | [changelog.txt](../changelog.txt) |

Internal refactors, test-only corrections, and package retry details remain in
the changelog when they do not change a public interface. Public compiler
options, predefined symbols, statements, runtime headers, platform limits, and
installation workflows are also linked from the generated manual.

<!-- end of release-history-1.10-to-1.20.md -->
