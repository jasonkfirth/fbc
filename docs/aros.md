<!--
    Project: FreeBASIC AROS port
    ---------------------------

    File: docs/aros.md

    Purpose:

        Document how to build, package, and qualify FreeBASIC for AROS.

    Responsibilities:

        - define the supported AROS architecture and library scope
        - explain the boundary between reusable m68k support and AROS policy
        - record the reproducible host, package, emulator, and test workflows
        - describe the current native-toolchain and emulator constraints

    This file intentionally does NOT contain:

        - implementation details already documented beside the source
        - generic policy for non-AROS operating systems
        - release claims that have not been verified in an AROS guest
-->

# FreeBASIC on AROS

The AROS port supports three target families:

- `aros-x86_64`, using the `pc-x86_64` AROS SDK
- `aros-arm`, using the Raspberry Pi ARM hard-float SDK
- `aros-m68k`, using the Amiga m68k 68000 soft-float SDK

Each target builds the compiler runtime, multithreaded runtime, gfxlib2,
sfxlib, and libffi. gfxlib2 uses a normal Intuition window with CyberGraphX
pixel transfer, so the AROS pointer remains above the game image. sfxlib
streams through `ahi.device` on its own worker and retains the null driver as
the fallback when AHI is unavailable.

The build uses the open AROS source tree, QEMU, and FS-UAE. It does not require
a proprietary Amiga ROM. The m68k workflow boots the AROS open ROM that is
built from the pinned source revision.

## Architecture and platform boundary

The m68k target is not an alias for AROS. The reusable target-family work is
kept in the shared compiler and build model:

- m68k target and CPU-family recognition
- big-endian, 32-bit code-generation identity
- classic m68k two-byte aggregate alignment
- generic m68k ELF handling

The following policy belongs specifically to the current AROS m68k SDK and
stays under AROS platform directories or AROS build scripts:

- the `-march=68000 -msoft-float` baseline
- AROS GCC helper workarounds and link ordering
- conversion from linked ELF to Amiga Hunk executables
- AROS open-ROM boot media and FS-UAE transport
- AROS m68k runtime replacements and test baselines

This split leaves the shared m68k work available to later operating-system
ports without imposing the AROS ABI or 68000 baseline on them.

AROS implementation files use the same replacement-directory model as the
other complete platform ports. Compiler policy is in `src/compiler/aros/`,
runtime replacements are in `src/rtlib/aros/`, gfxlib2 is in
`src/gfxlib2/aros/`, sfxlib is in `src/sfxlib/aros/`, and CRT declarations are
in `inc/aros/`. AROS SDK architecture flags, including the m68k 68000
soft-float baseline, are in `mk/aros/`. Shared sources contain target detection
and dispatch, not the AROS implementations.

## One-command build and packaging

On Debian or Ubuntu, run:

```sh
./build_scripts/debianubuntu-build-freebasic-aros.sh
```

The script installs its host prerequisites, checks out the pinned AROS core
and contrib revisions, builds all three SDKs and cross toolchains, builds
target libffi, builds FreeBASIC and its libraries, builds the AROS-hosted
development tools, creates native compiler packages, and builds AROS boot
media. The complete native compile-and-link path is qualified on x86_64 and
m68k. ARM packages are built and installed in a clean guest, but the complete
native driver chain remains experimental as described below. The m68k cross
toolchain also builds the compiler, runtime, graphics, sound, tests, examples,
and game packages used for the larger release qualification runs.

Useful development forms are:

```sh
# Reuse the installed Debian or Ubuntu packages.
./build_scripts/debianubuntu-build-freebasic-aros.sh --skip-deps

# Rebuild FreeBASIC with existing AROS SDKs and omit boot media.
./build_scripts/debianubuntu-build-freebasic-aros.sh \
  --skip-deps --skip-aros-build --no-images

# Work on one target.
./build_scripts/debianubuntu-build-freebasic-aros.sh \
  --skip-deps --targets m68k
```

The installable archives are written to `out/aros/packages/` as one `.pkg` and
one individually downloadable `.zip` for FreeBASIC and its matching
Developer support package per architecture. Each ZIP contains the AROS package
and its installation README. The FreeBASIC package contains the native `fbc`,
headers, normal and multithreaded runtime libraries, gfxlib2, sfxlib, and
libffi. The companion package installs the matching `SYS:Developer` tree with
GCC, binutils, headers, libraries, and GCC support programs.

The `.pkg` file is a bzip2-wrapped PKG1 stream, matching the input expected by
the native AROS `C:Unpack` command. Package construction decompresses every
finished archive, validates its record boundaries, and compares the extracted
tree with the staged input before creating the download ZIP.

Install the matching Developer package before the FreeBASIC package. This
keeps GCC and binutils in AROS's standard `Developer:` location while keeping
the FreeBASIC compiler package independently downloadable.

Native-compilation qualification covers x86_64 and m68k. After assigning the
matching `Developer:` and `FreeBASIC:` trees, those guests build and run a new
BASIC program with the normal compiler interface:

```text
fbc -target aros -m hello hello.bas
hello
```

On ARM, use `SYS:FreeBASIC/bin/fbc` instead of the bare command. The current
ARM FAT handler can block while `Path` compares a newly installed directory
lock. The package startup file therefore leaves the shell path alone and sets
absolute GCC, assembler, linker, and GCC support-program paths. x86_64 and
m68k retain the ordinary `fbc` command shown above. The native ARM compiler
and GCC front end also need a larger shell stack:

```text
Stack 16777216
SYS:FreeBASIC/bin/fbc -target aros -m hello hello.bas
```

This ARM command is not yet a qualified end-to-end path. The installed fbc
front end and the individual GCC stages work, but a nested `fbc -> gcc -> cc1`
invocation can lose or stall its command stream at the AROS process boundary.
The package is published so the ARM port and its separate tools can be tested,
but it should be treated as experimental until an ordinary fbc invocation can
compile and link in a clean guest.

The x86_64 and ARM paths retain the native GCC C and C++ front ends. The m68k
path converts fbc, GCC, assembler, linker, collect-aros, and cc1 into loadable
Hunk programs. AROS process-boundary ownership and collect-aros temporary-file
handling are patched at the AROS delivery boundary. This lets one ordinary fbc
invocation compile C, assemble it, link it, convert the output to Hunk, and run
the result inside the m68k guest.

On ARM, GCC locates its private compiler stages below
`SYS:Developer/libexec/gcc`, probes executables and directories through native
dos.library locks instead of the POSIX `stat()` bridge, and runs each stage
synchronously with `SystemTags()`. These changes remove the path-probe failures
and let the compiler components run directly, but they do not solve the nested
command-stream failure described above. The ARM console teardown also emits
reset sequences only when the output handle is interactive, so it does not ask
the POSIX stdout bridge to seek through a redirected FAT file.

The checked native m68k smoke test is:

```sh
./build_scripts/aros-run-native-m68k-smoke.sh
```

Generated C uses `-O0`, and the guest runs each native tool with a 1 MiB stack.
FS-UAE is configured with its maximum 1 GiB Zorro III board, although the
current AROS m68k allocator exposes only part of that region to normal process
allocations. Larger guest-native builds may therefore need to be divided into
batches. The cross toolchain remains the practical path for full release-scale
matrices, but it is no longer compensating for a missing native compiler path.

On x86_64, qualification also exercises the lower-level GAS64 diagnostic path
directly. That path caught an otherwise silent Microsoft-versus-SysV x86-64
calling-convention mismatch, but it is no longer a fallback for an unreliable
driver. The ordinary one-command `fbc` path is the native compiler criterion
on x86_64 and m68k. ARM retains the failed nested-driver evidence alongside its
successful package-install and direct-component checks. Cross-built guest
execution remains the scalable criterion for the full fbctests,
Exampleageddon, and OMA matrices.

## Clean package installation

Test the actual package pair, rather than files copied from the build tree,
with:

```sh
./build_scripts/aros-test-packages.sh \
  --targets x86_64,m68k,arm
```

The runner removes the prebuilt `Developer` tree from fresh x86_64 and m68k
boot media and constructs the ARM SD image without `Developer` or FreeBASIC.
Inside each new guest, the native `C:Unpack` command installs the matching
Developer and FreeBASIC packages. On x86_64 and m68k, the installed `fbc`, GCC,
assembler, linker, and m68k Hunk converter then build a new BASIC program. A
full pass requires the program's distinctive return status, its host-visible
marker, the expected AROS executable format, the package hashes, and the guest
installation logs. ARM currently passes both native package installations and
the installed-tree checks, then records the known nested-driver failure rather
than reporting a complete compiler pass. The test medium includes two small
target-native helpers for serial evidence and bounded file copies because the
minimal ARM image does not supply the ordinary `Copy` or `Type` shell commands.

The emulated Raspberry Pi currently has no usable AROS public screen. The AROS
source patch used by the build therefore lets `C:Unpack` omit its progress
window when no public screen exists. Desktop systems retain the normal window;
only the presentation is skipped in the serial-only case. Test evidence is
kept below `out/aros/package-test/`.

## Full fbctests qualification

Run all fbcunit directories on all three AROS targets with:

```sh
./build_scripts/aros-run-fbctests.sh \
  --targets x86_64,m68k,arm --timeout 1800
```

The x86_64 runner uses 2 GiB by default and rejects values below 256 MiB. The
QEMU Raspberry Pi 2 model exposes its fixed 1 GiB layout. AROS m68k currently
cannot use its entire emulated expansion-memory region for early `LoadSeg`
allocations, so that target uses one top-level test directory per executable.
This is a memory-bounded batching policy, not a reduction in test coverage.

Use `--resume` to continue after a host interruption. Completion records are
accepted only when their batch manifest still matches. Logs and state are
kept below `out/aros/fbctests/`.

## Exampleageddon

Compile the complete example inventory and run every example classified as
self-contained with:

```sh
./build_scripts/aros-run-exampleageddon.sh \
  --targets x86_64,m68k,arm --timeout 900
```

The runner retains compile failures for interactive, platform-specific,
external-library, and intentionally failing examples as coverage information.
Its unattended pass condition applies to every self-contained example. CSV
and Markdown reports, guest logs, and resumable state are written below
`out/aros/exampleageddon/`.

## OMA games

Build and package every OMA or OpenSlicks source tree present in the workspace:

```sh
./build_scripts/aros-build-oma-games.sh \
  --targets x86_64,m68k,arm
```

Each game and architecture receives an AROS `.pkg` plus an individually
downloadable `.zip` below `out/aros/packages/oma/`. The manifest records
missing source trees explicitly rather than silently reducing the requested
matrix.

Launch-qualify the built games with:

```sh
./build_scripts/aros-run-oma-games.sh \
  --targets x86_64,m68k,arm --skip-build
```

A launch pass proves that the executable loads, resolves its staged assets,
opens the gfxlib2 path, and remains alive for the configured stability period.
These interactive games have no deterministic gameplay API, so screenshots
and guest logs are preserved for review below `out/aros/oma/tests/`.

## Boot and test media

The main build produces its architecture media under the corresponding AROS
build directory:

- `out/aros/build-pc-x86_64/distfiles/aros-pc-x86_64.iso`
- `out/aros/build-amiga-m68k/distfiles/aros-amiga-m68k.iso`
- `out/aros/build-raspi-armhf/distfiles/aros-arm-raspi.img`

The test runners derive temporary media from these files and keep their test
results outside the source checkout. x86_64 and ARM use QEMU. m68k uses FS-UAE
with the AROS-built open ROM. Linked m68k ELF files are converted to Hunk only
at the AROS delivery boundary, including the native compiler placed in its
installable package.

<!-- end of docs/aros.md -->
