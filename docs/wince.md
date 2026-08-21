# FreeBASIC Windows CE port

File: `docs/wince.md`

Purpose: document the Windows CE target, reproducible build environment,
emulator boundary, qualification workflow, and release-package contract.

Responsibilities:

- identify the supported Windows CE ARM and MIPS ABIs and supply chains
- explain how to prepare the non-redistributable emulator image
- provide complete and resumable build/test commands
- describe rtlib, gfxlib2, sfxlib, compiler, and application packages
- record what each automated qualification result proves

This file intentionally does not document Windows desktop or generic compiler
internals.

## Supported target

| FreeBASIC target | GNU target triplet | CPU baseline | Floating point | Executable format |
| --- | --- | --- | --- | --- |
| `wince-arm` | `arm-mingw32ce` | ARMv4T | software | PE/COFF for Windows CE |
| `wince-mips` | `mipsel-wince-pe` | little-endian MIPS III O32 | software | MIPS PE/COFF for Windows CE |

The compiler, runtime, graphics, sound, headers, build flags, and tests keep
Windows CE behavior in their corresponding `wince/` directories. Shared
compiler code contains only the target-neutral dispatch needed to reach those
replacements.

The target uses the maintained CeGCC 9.3 ARM toolchain from
`ghcr.io/enlyze/windows-ce-build-environment-arm`, pinned by digest in
`build_scripts/wince/Dockerfile`. No CeGCC packages are installed from the
Ubuntu host repositories.

The MIPS target builds the removed GNU MIPS PE BFD backend from pinned CeGCC
source, then uses current Clang for code generation. This is intentionally not
the Linux MIPS ELF toolchain. Packages such as `gcc-mips-linux-gnu` are neither
required nor suitable for Windows CE PE output.

## Host requirements

Install the complete ARM and MIPS host prerequisites if they are not already
present:

```console
sudo apt-get update
sudo apt-get install -y build-essential git docker.io flex bison texinfo \
    clang llvm rsync xz-utils
```

The ARM cross compiler, archive tools, Wine, Xvfb, and emulator automation
dependencies live in containers. MIPS PE binutils are built from source. The
repository does not need Ubuntu MIPS cross-GCC packages.

## Emulator preparation

CERF is public software, but the Windows Mobile ROM remains a Microsoft asset
that FreeBASIC cannot redistribute. Download
`Windows Mobile 6 Standard Images (USA).msi` from
the [official Microsoft Download Center](https://www.microsoft.com/en-us/download/details.aspx?id=7974), then run:

```console
./build_scripts/wince/prepare-arm-emulator.sh \
    --rom-msi "$HOME/Downloads/Windows Mobile 6 Standard Images (USA).msi"
```

The preparation script:

1. Builds the pinned Wine/Xvfb support image.
2. Downloads and authenticates the public [CERF 6.7 release](https://github.com/gweslab/cerf/releases/tag/6.7).
3. Authenticates the local Microsoft MSI.
4. Extracts the qualified QVGA ARM image without installing the MSI.
5. Creates `out/wince/emulator/cerf` with `share/` and `logs/` directories.

The ROM is never copied into a FreeBASIC package. Windows Mobile 6 is outside
Microsoft's support lifecycle, so this emulator is a compatibility test
environment rather than a supported Microsoft development product.

For MIPS, obtain a licensed ROM for a CERF-supported MIPS III or MIPS IV
device. The Casio Cassiopeia EM-500 is the default because CERF supports its
guest additions, graphics, input, and sound. Prepare a separate emulator tree:

```console
./build_scripts/wince/prepare-mips-emulator.sh \
    --rom /path/to/licensed-mips-rom.bin \
    --board-id casio_cassiopeia_em500
```

The official CERF launcher is the normal ROM bundle downloader. The raw-ROM
entry point exists so a licensed local image can still be used when the public
bundle catalog is unavailable. Neither route places a ROM in this repository
or its packages.

## Complete build and qualification

From the FreeBASIC source root, run:

```console
./build_scripts/debianubuntu-build-freebasic-wince.sh
```

The workflow copies the current source, including uncommitted port work, into
`out/wince/work`. Generated objects and earlier output are excluded. It then:

1. Builds a current Linux x86_64 host compiler.
2. Downloads, authenticates, patches, and builds libffi 3.5.2 for ARMv4T.
3. Builds normal and multithreaded rtlib, gfxlib2, and sfxlib.
4. Compiles PE/COFF smoke applications for the runtime, graphics, and sound.
5. Creates and structurally validates the relocatable cross-SDK archives.
6. Runs a package-built executable in Windows CE.
7. Builds and runs every top-level fbctests directory in bounded batches.
8. Compiles Exampleageddon and runs every self-contained example in batches.
9. Builds, individually packages, and launch-qualifies every available OMA
   game.

Parallel build jobs default to two to avoid unnecessary memory pressure:

```console
./build_scripts/debianubuntu-build-freebasic-wince.sh --jobs 1
```

To continue an interrupted run without rebuilding compatible artifacts:

```console
./build_scripts/debianubuntu-build-freebasic-wince.sh \
    --skip-toolchain-image \
    --skip-emulator-image \
    --reuse-worktree \
    --incremental \
    --resume
```

Individual phases can be omitted with `--skip-package`, `--skip-fbctests`,
`--skip-exampleageddon`, or `--skip-oma`. These options are intended for
focused development; a release qualification runs all phases.

The MIPS workflow is separate because its toolchain is Clang plus MIPS PE
binutils instead of ARM CeGCC:

```console
./build_scripts/wince/build-mips-libraries.sh --jobs 2
./build_scripts/wince/run-mips-fbctests.sh --build-only --jobs 2
./build_scripts/wince/run-exampleageddon.sh \
    --arch mips --compile-only --jobs 2
./build_scripts/wince/build-oma-games.sh --arch mips
./build_scripts/wince/package-mips-cross-sdk.sh
```

After preparing the MIPS emulator, omit `--build-only` and `--compile-only` to
run fbctests and Exampleageddon in bounded guest batches. The test executables
remain split by top-level directory to fit older devices with limited memory.

## Runtime and media behavior

The Windows CE runtime uses UTF-16 platform paths, CE-compatible CRT shims,
WinSock, native threads, and the platform's limited process-launch surface.
The executable-path fallback searches the shared filesystem when the CE image
does not report the launched module path correctly.

gfxlib2 presents the FreeBASIC framebuffer through a GDI window. Input is read
from the same window, and cursor requests use the native system cursor so it
remains visible above framebuffer updates. The host Task Manager is only used
as a process-launch bridge and is hidden immediately after launch.

sfxlib uses WinMM for device output and retains its null fallback. Its format
decoders and MIDI FM path are included in normal and multithreaded libraries.
Debug output is disabled from normal display and audio update paths so it
cannot disturb the guest framebuffer.

## Qualification results

The current Windows CE ARM result set below `out/wince` records:

- fbctests: 5,224 tests in 37 directories, zero failures and zero errors
- native Linux regression: 2,397 tests and 1,155,542 assertions, zero failures
- Exampleageddon: 651 of 651 self-contained examples executed successfully
- OMA: 9 of 9 available games built, packaged, and remained alive through a
  15-second startup interval

Six OMA catalog entries are recorded as missing because those source trees are
not present in this checkout. The OMA result is a launch qualification, not
deterministic gameplay automation. Behold remains alive without opening a
visible game window, and Duel 999 presents a stable black framebuffer during
the observation interval; those screenshots remain in the evidence tree.

The current Windows CE MIPS cross-build result set records:

- fbctests cross-build: 37 of 37 directory executables in MIPS PE format
- Exampleageddon cross-build: 651 of 651 self-contained examples, with 1,248
  total examples linked
- media link coverage: 81 gfx-family and 57 sfxlib examples linked
- OMA: 9 of 9 locally available games built as MIPS PE and packaged
- compiler SDK: both archives extracted, byte-compared, checksum-verified,
  and used for runtime, gfxlib2, and sfxlib smoke builds

These MIPS entries are cross-build results, not guest execution results. Full
guest fbctests, Exampleageddon, and OMA launch qualification require the
user-supplied ROM described above.

## Compiler cross-SDK packages

`out/wince/packages/compiler` contains individually downloadable `tar.xz` and
ZIP archives plus SHA-256 manifests. Each ARM archive contains:

- the Linux x86_64 `bin/fbc` compiler
- `bin/fbc-wince-arm`, which selects the target and pinned ABI contract
- the complete `include/freebasic` tree
- normal and multithreaded rtlib
- normal and multithreaded gfxlib2 and sfxlib
- target libffi and its public headers and license
- Windows CE port documentation

The MIPS package additionally contains the pinned `mips-wince-pe-*` binary
tools, Windows CE headers and import libraries, compiler-rt builtins, and
`bin/fbc-wince-mips`. Its only compiler prerequisite is host Clang:

```console
sudo apt-get install -y clang
FreeBASIC-*-wince-mips-cross-linux-x86_64/bin/fbc-wince-mips \
    hello.bas -x hello.exe
```

After extraction, place the CeGCC `arm-mingw32ce-*` programs on `PATH` and
compile with:

```console
FreeBASIC-*/bin/fbc-wince-arm hello.bas -x hello.exe
```

This is intentionally a Linux-hosted cross-SDK. A useful on-device compiler
would also require a maintained native Windows CE GCC/binutils installation,
which the target does not provide. The archive therefore does not pretend that
copying `fbc` to a device is enough to compile there.

## OMA application packages

Each available game has its own deployable ZIP below
`out/wince/packages/oma/arm`. A package contains one Windows CE executable and
only that game's required runtime assets. The build script extracts every ZIP
and byte-compares it with the staging tree before declaring it ready.

Build packages without launching the emulator:

```console
./build_scripts/wince/build-oma-games.sh
./build_scripts/wince/build-oma-games.sh --arch mips
```

Re-run launch qualification against those exact packages:

```console
./build_scripts/wince/run-oma-games.sh --skip-build --stability 15
./build_scripts/wince/run-mips-oma-games.sh --skip-build --stability 15
```

<!-- end of docs/wince.md -->
