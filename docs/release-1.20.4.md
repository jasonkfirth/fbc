# FreeBASIC 1.20.4 release notes

FreeBASIC 1.20.4 completes the large portability and runtime expansion that
began during 1.20.2 development and continued through 1.20.3. It is more than
a packaging refresh: the compiler and libraries gained new target families,
new built-in graphics and sound work, portable serial controls, extensive
platform backends, and a much broader qualification system.

The detailed chronological record remains in [changelog.txt](../changelog.txt).
This page groups the important user-visible changes by subject.

## Compiler and runtime

- First-class compiler support was added for NuttX RISC-V, RISC OS, AROS
  x86-64/ARM/m68k, Windows CE ARM/MIPS, and the four Linux MIPS32/MIPS64
  endian and ABI combinations.
- Windows 95, 98, and ME now use an isolated ANSI filesystem and older CRT
  compatibility path. A Windows 95 OSR2 i486 guest passed the native compiler
  smoke, all 37 fbctest executables, and 651 unattended examples.
- Cross-target WSTRING handling now uses the target wchar layout. This fixes
  UTF-16 surrogate pairs and constant folding when the compiler host and target
  use different wchar widths.
- `OPEN TCP SERVER`, bounded `TCP ACCEPT`, EOF/EOC separation, and socket
  declarations were completed or corrected across the supported native ports.
- `OPEN COM` gained the portable `fbcom.bi` status and line-control API. See
  [Portable serial control](serial.md).
- Compiler and runtime fixes cover REDIM lookahead, packed and unaligned data,
  constant comparisons, object/linker inputs, Win32 stack probing, DOS input,
  redirected WSTRING output, file allocation paths, and many target-specific
  ABI details.

## Graphics

- gfxlib2 remains the default graphics runtime. It now includes touch helpers,
  Xbox-style controller polling with `GETXPAD`, many new platform backends, and
  runtime-selected SSE2 or NEON acceleration for common pixel operations.
- gfxlib3 is a new opt-in GPU runtime for Windows, Linux/X11, and Android. It
  owns rendering on a dedicated thread, uses Vulkan with OpenGL fallback on
  desktop systems, uses OpenGL ES on Android, and has a headless reference
  backend. Select it with `fbc -gfx3` or the source-level selector documented
  in the [gfxlib3 guide](gfxlib3/README.md).
- gfxlib3 implements the normal FreeBASIC and QB graphics command surface,
  including pages, palettes, the graphical console, input, drawing primitives,
  images, GET/PUT, BLOAD/BSAVE, SCREENPTR, and SCREENCONTROL.
- gfxlib3 adds GPU-resident surfaces, explicit mapping and transfer, scaled and
  rotated blits, Mode 7 projection, render-thread callbacks, and resizable
  screen events through `fbgfx3.bi`.
- Historical `SCREEN` modes 3 through 6 are now available in gfxlib3:
  Hercules 720x348 monochrome, Olivetti/AT&T 640x400 monochrome, PCjr/Tandy
  320x200 16-colour, and PCjr/Tandy 640x200 4-colour.
- gfxlib3 has an independent checked PNG codec for BLOAD, BSAVE, screen pages,
  and direct GPU asset loading.

## Sound

- The built-in sfxlib command surface covers generated sound, PLAY strings,
  short effects, music files, MIDI, output device selection, and capture.
- A bounded 32-voice two-operator FM synthesizer supplies all 128 General MIDI
  program families plus common channel controls and percussion when native
  MIDI is unavailable. `SFXLIB_MIDI_DRIVER=fm` selects it explicitly.
- The optional `sfxlib_raw.bi` and `sfxlib_effects.bi` headers expose raw float
  output, final-output WAV recording, underrun counters, sample-rate queries,
  and a whole-mix stereo ping-pong echo.
- The mixer and PCM conversions now use runtime-selected x86 SSE2/MMX or ARM
  NEON paths where safe, with scalar behavior retained for unsupported CPUs.
  Mixing, FM synthesis, echo, diagnostics, and ring transfers work in blocks.

The generated manual contains the full sound command syntax. Runnable examples
and diagnostic environment variables are described in the
[sfxlib example guide](../examples/sfxlib/README.md).

## Platforms and packages

- Native or emulated package qualification now covers current Debian, Ubuntu,
  Raspbian, Alpine/postmarketOS, rpm-based Linux, FreeBSD, OpenBSD, NetBSD,
  DragonFly BSD, Haiku, illumos, Cygwin, Windows, DOS, macOS, and the optional
  JavaScript, Android, Wii, Xbox, and NuttX targets.
- The Linux release matrix is intentionally limited to the current useful
  stable and development distributions instead of rebuilding obsolete distro
  combinations.
- Debian and Ubuntu split the compiler, runtime, bindings, examples,
  documentation, and optional target SDKs into installable packages. Ubuntu
  Resolute amd64 carries the qualified NuttX, Wii, Android, JavaScript, and
  Xbox package outputs.
- Windows-hosted standalone packages are available for Win32, Win64,
  Win32-on-ARM64, Android, JavaScript, Wii, and Xbox. The exact current files
  are listed by the repository rather than frozen in this document.
- A native ARM64 Termux plus Ubuntu PRoot bootstrap can build an Android APK on
  an Android phone without QEMU or an x86-64 NDK host toolchain.

See [Package repository and installation](packages.md) for current locations
and integrity checks.

## Quality and compatibility work

- Strict GCC and Clang warning-as-error builds, Cppcheck, Clang-Tidy, fblint,
  fbctests, Exampleageddon, package installation tests, and platform smoke tests
  now cover much more of the compiler and runtime tree.
- More than 700 examples were made safer and more portable while preserving
  examples whose purpose is to demonstrate an error, dialect rule, or unsafe
  interface.
- Allocation sizes, string bounds, file reads, generated tables, temporary
  ownership, platform handles, and cleanup paths were hardened throughout the
  compiler, rtlib, gfxlib2, gfxlib3, sfxlib, and build tools.

## Experimental boundaries

- gfxlib3 is opt-in and currently windowed on Windows, Linux/X11, and Android.
  DOS, Haiku, JavaScript, NuttX, Wii, Xbox, and other platforms continue to use
  gfxlib2 unless a gfxlib3 adapter is added.
- The AROS ARM installed compiler components work individually, but its nested
  `fbc -> gcc -> cc1` driver path remains an explicitly recorded limitation.
- Windows CE qualification is ROM-free cross-build and static inspection unless
  a suitable licensed emulator image is supplied.
- Linux MIPS toolchains remain qualification targets and are not presented as
  complete primary release packages.
- Hardware-dependent graphics, controller, serial, audio, and firmware paths
  can require the named device, SDK, ROM, or legal console firmware. Headless
  tests do not substitute for those physical checks.

<!-- end of release-1.20.4.md -->
